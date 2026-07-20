#include "FenixEfbClient.h"

#include <algorithm>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include "../logging/LogMacros.h"

namespace
{
    constexpr auto kGraphQlUrl = "http://localhost:8083/graphql";
    constexpr auto kLoadsheetUrlPrefix = "http://localhost:8083/fenix/loadsheet/generate?type=";
    constexpr int kTransferTimeoutMs = 2000;
    constexpr int kPollFailuresBeforeUnavailable = 2;

    QString AliasFor(const std::string& name)
    {
        return QString::fromStdString(name).remove(QLatin1Char('.'));
    }

    QByteArray WrapRequest(const QString& query, const QJsonObject& variables)
    {
        QJsonObject payload;
        payload.insert(QStringLiteral("variables"), variables);
        payload.insert(QStringLiteral("query"), query);

        return QJsonDocument(payload).toJson(QJsonDocument::Compact);
    }

    QNetworkRequest JsonRequest(const char* url)
    {
        QNetworkRequest request{QUrl(QString::fromLatin1(url))};
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

        return request;
    }
}

FenixEfbClient::FenixEfbClient() = default;

FenixEfbClient::~FenixEfbClient() = default;

QByteArray FenixEfbClient::BuildValuesQuery(const std::vector<std::string>& names)
{
    QString fields;
    for (const std::string& name : names)
    {
        fields += QStringLiteral("%1: dataRef(name: \"%2\") { value __typename } ")
            .arg(AliasFor(name), QString::fromStdString(name));
    }

    return WrapRequest(QStringLiteral("{ dataRef { %1__typename } }").arg(fields), {});
}

std::optional<std::map<std::string, QJsonValue>> FenixEfbClient::ParseValuesResponse(
    const QByteArray& body, const std::vector<std::string>& names)
{
    const QJsonObject dataRefs = QJsonDocument::fromJson(body).object()
                                                              .value(QStringLiteral("data")).toObject()
                                                              .value(QStringLiteral("dataRef")).toObject();
    if (dataRefs.isEmpty())
    {
        return std::nullopt;
    }

    std::map<std::string, QJsonValue> values;
    for (const std::string& name : names)
    {
        const QJsonValue value = dataRefs.value(AliasFor(name)).toObject().value(QStringLiteral("value"));
        if (!value.isUndefined() && !value.isNull())
        {
            values[name] = value;
        }
    }

    return values;
}

QByteArray FenixEfbClient::BuildWriteMutation(const std::string& operation,
                                              const std::string& typeName,
                                              const std::string& name,
                                              const QJsonValue& value)
{
    const QString alias = AliasFor(name);
    const QString query = QStringLiteral("mutation ($%1: %2!) { dataRef { %3(name: \"%4\", value: $%1) __typename } }")
        .arg(alias, QString::fromStdString(typeName), QString::fromStdString(operation),
             QString::fromStdString(name));

    QJsonObject variables;
    variables.insert(alias, value);

    return WrapRequest(query, variables);
}

void FenixEfbClient::Subscribe(const std::string& name)
{
    if (std::ranges::find(subscriptions_, name) == subscriptions_.end())
    {
        subscriptions_.push_back(name);
    }
}

void FenixEfbClient::Poll()
{
    if (subscriptions_.empty() || pollReply_ != nullptr)
    {
        return;
    }

    QNetworkAccessManager& network = EnsureNetwork();
    pollReply_ = network.post(JsonRequest(kGraphQlUrl), BuildValuesQuery(subscriptions_));
    if (pollReply_ == nullptr)
    {
        RegisterPollResult(false);

        return;
    }

    QObject::connect(pollReply_, &QNetworkReply::finished, &network, [this] { OnPollFinished(); });
}

void FenixEfbClient::OnPollFinished()
{
    QNetworkReply* reply = pollReply_;
    pollReply_ = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        RegisterPollResult(false);

        return;
    }

    const auto values = ParseValuesResponse(reply->readAll(), subscriptions_);
    if (!values.has_value())
    {
        RegisterPollResult(false);

        return;
    }

    values_ = *values;
    RegisterPollResult(true);
}

void FenixEfbClient::RegisterPollResult(const bool succeeded)
{
    if (succeeded)
    {
        if (!available_)
        {
            LOG_INFO("Fenix EFB reachable: GraphQL responding at %s", kGraphQlUrl);
        }

        consecutivePollFailures_ = 0;
        available_ = true;

        return;
    }

    ++consecutivePollFailures_;
    if (available_ && consecutivePollFailures_ >= kPollFailuresBeforeUnavailable)
    {
        available_ = false;

        LOG_WARN("Fenix EFB unreachable: pausing EFB writes until it responds again");
    }
}

bool FenixEfbClient::IsAvailable() const
{
    return available_;
}

double FenixEfbClient::GetNumber(const std::string& name, const double defaultValue) const
{
    const auto it = values_.find(name);
    if (it == values_.end())
    {
        return defaultValue;
    }

    if (it->second.isBool())
    {
        return it->second.toBool() ? 1.0 : 0.0;
    }

    return it->second.toDouble(defaultValue);
}

std::vector<bool> FenixEfbClient::GetBoolArray(const std::string& name) const
{
    const auto it = values_.find(name);
    if (it == values_.end() || !it->second.isArray())
    {
        return {};
    }

    std::vector<bool> array;
    for (const auto& entry : it->second.toArray())
    {
        array.push_back(entry.toBool());
    }

    return array;
}

void FenixEfbClient::SetFloat(const std::string& name, const double value)
{
    SendMutation("writeFloat", "Float", name, QJsonValue(value));
}

void FenixEfbClient::SetBool(const std::string& name, const bool value)
{
    SendMutation("writeBool", "Boolean", name, QJsonValue(value));
}

void FenixEfbClient::SetString(const std::string& name, const std::string& value)
{
    SendMutation("writeString", "String", name, QJsonValue(QString::fromStdString(value)));
}

void FenixEfbClient::SendMutation(const std::string& operation, const std::string& typeName,
                                  const std::string& name, const QJsonValue& value)
{
    if (!available_)
    {
        return;
    }

    const QNetworkReply* reply =
        EnsureNetwork().post(JsonRequest(kGraphQlUrl), BuildWriteMutation(operation, typeName, name, value));
    if (reply != nullptr)
    {
        QObject::connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    }
}

void FenixEfbClient::RequestLoadsheet(const std::string& type)
{
    if (!available_)
    {
        return;
    }

    QNetworkRequest request{QUrl(QString::fromLatin1(kLoadsheetUrlPrefix) + QString::fromStdString(type))};
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    const QNetworkReply* reply = EnsureNetwork().post(request, QByteArray());
    if (reply != nullptr)
    {
        QObject::connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    }

    LOG_INFO("Requested Fenix %s loadsheet", type.c_str());
}

QNetworkAccessManager& FenixEfbClient::EnsureNetwork()
{
    if (network_ == nullptr)
    {
        network_ = std::make_unique<QNetworkAccessManager>();
        network_->setTransferTimeout(kTransferTimeoutMs);
    }

    return *network_;
}

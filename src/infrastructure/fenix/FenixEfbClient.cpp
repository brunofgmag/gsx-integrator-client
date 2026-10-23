#include "FenixEfbClient.h"

#include <algorithm>
#include <optional>
#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include "../logging/LogMacros.h"
#include "../probe/ProbeLog.h"

namespace
{
    constexpr auto kGraphQlUrl = "http://localhost:8083/graphql";
    constexpr auto kLoadsheetUrlPrefix = "http://localhost:8083/fenix/loadsheet/generate?type=";
    constexpr int kTransferTimeoutMs = 2000;
    constexpr int kPollFailuresBeforeUnavailable = 2;
    constexpr int kPollsBetweenSettingsRequests = 10;
    constexpr auto kSettingsQuery = "{ settings { softwareVersion state { settings { key value } } } }";

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

    QNetworkRequest JsonRequest(const QString& url)
    {
        QNetworkRequest request{QUrl(url)};
        request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

        return request;
    }

    std::optional<int> MajorVersionOf(const QString& softwareVersion)
    {
        bool parsed = false;
        const int major = softwareVersion.section(QLatin1Char('.'), 0, 0).toInt(&parsed);
        if (!parsed)
        {
            return std::nullopt;
        }

        return major;
    }

    QString ShownValue(const QJsonValue& value)
    {
        const QByteArray encoded = QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact);

        return QString::fromUtf8(encoded.mid(1, encoded.size() - 2));
    }
}

FenixEfbClient::FenixEfbClient()
    : endpoint_(QString::fromLatin1(kGraphQlUrl))
{
}

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

QByteArray FenixEfbClient::BuildSettingsQuery()
{
    return WrapRequest(QString::fromLatin1(kSettingsQuery), {});
}

std::optional<FenixEfbClient::SettingsSnapshot> FenixEfbClient::ParseSettingsResponse(
    const QByteArray& body, const std::vector<std::string>& keys)
{
    const QJsonObject settings = QJsonDocument::fromJson(body).object()
                                                              .value(QStringLiteral("data")).toObject()
                                                              .value(QStringLiteral("settings")).toObject();
    const std::optional<int> major = MajorVersionOf(settings.value(QStringLiteral("softwareVersion")).toString());
    if (!major.has_value())
    {
        return std::nullopt;
    }

    SettingsSnapshot snapshot;
    snapshot.softwareMajorVersion = *major;

    const QJsonArray entries = settings.value(QStringLiteral("state")).toObject()
                                       .value(QStringLiteral("settings")).toArray();
    for (const QJsonValue& entry : entries)
    {
        const QJsonObject setting = entry.toObject();
        const std::string key = setting.value(QStringLiteral("key")).toString().toStdString();
        const QJsonValue value = setting.value(QStringLiteral("value"));
        if (value.isString() && std::ranges::find(keys, key) != keys.end())
        {
            snapshot.values[key] = value.toString().toStdString();
        }
    }

    return snapshot;
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

void FenixEfbClient::SubscribeSetting(const std::string& key)
{
    if (std::ranges::find(settingSubscriptions_, key) == settingSubscriptions_.end())
    {
        settingSubscriptions_.push_back(key);
    }
}

void FenixEfbClient::Poll()
{
    PollSettings();

    if (subscriptions_.empty() || pollReply_ != nullptr)
    {
        return;
    }

    QNetworkAccessManager& network = EnsureNetwork();
    pollReply_ = network.post(JsonRequest(endpoint_), BuildValuesQuery(subscriptions_));
    if (pollReply_ == nullptr)
    {
        RegisterPollResult(false);

        return;
    }

    QObject::connect(pollReply_, &QNetworkReply::finished, &network, [this] { OnPollFinished(); });
}

void FenixEfbClient::PollSettings()
{
    if (settingSubscriptions_.empty())
    {
        return;
    }

    if (pollsUntilSettings_ > 0)
    {
        --pollsUntilSettings_;

        return;
    }

    if (settingsReply_ != nullptr)
    {
        return;
    }

    pollsUntilSettings_ = kPollsBetweenSettingsRequests - 1;

    QNetworkAccessManager& network = EnsureNetwork();
    settingsReply_ = network.post(JsonRequest(endpoint_), BuildSettingsQuery());
    if (settingsReply_ == nullptr)
    {
        return;
    }

    QObject::connect(settingsReply_, &QNetworkReply::finished, &network, [this] { OnSettingsFinished(); });
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

void FenixEfbClient::OnSettingsFinished()
{
    QNetworkReply* reply = settingsReply_;
    settingsReply_ = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        return;
    }

    const auto snapshot = ParseSettingsResponse(reply->readAll(), settingSubscriptions_);
    if (!snapshot.has_value())
    {
        return;
    }

    softwareMajorVersion_ = snapshot->softwareMajorVersion;
    settings_ = snapshot->values;
}

void FenixEfbClient::RegisterPollResult(const bool succeeded)
{
    if (succeeded)
    {
        if (!available_)
        {
            LOG_INFO("Fenix EFB reachable: GraphQL responding at %s", qUtf8Printable(endpoint_));
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

std::string FenixEfbClient::GetString(const std::string& name, const std::string& defaultValue) const
{
    const auto it = values_.find(name);
    if (it == values_.end() || !it->second.isString())
    {
        return defaultValue;
    }

    return it->second.toString().toStdString();
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

std::string FenixEfbClient::GetSetting(const std::string& key, const std::string& defaultValue) const
{
    const auto it = settings_.find(key);

    return it != settings_.end() ? it->second : defaultValue;
}

std::optional<int> FenixEfbClient::GetSoftwareMajorVersion() const
{
    return softwareMajorVersion_;
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
        EnsureNetwork().post(JsonRequest(endpoint_), BuildWriteMutation(operation, typeName, name, value));
    if (reply == nullptr)
    {
        return;
    }

    QObject::connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    ReportWrite(name, value);
}

void FenixEfbClient::ReportWrite(const std::string& name, const QJsonValue& value)
{
    if (!probe::IsOn())
    {
        return;
    }

    const QString shown = ShownValue(value);
    const std::optional<int> count = writeMemo_.Record(name, shown, QDateTime::currentMSecsSinceEpoch());
    if (!count)
    {
        return;
    }

    probe::Line(probe::Channel::Writes, QStringLiteral("graphql %1=%2 n=%3")
                .arg(QString::fromStdString(name), shown)
                .arg(*count));
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

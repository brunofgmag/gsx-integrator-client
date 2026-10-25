#include "FslabsMcduClient.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include "../logging/LogMacros.h"
#include "../probe/ProbeLog.h"

namespace
{
    constexpr auto kBaseUrl = "http://127.0.0.1:8080";
    constexpr auto kRightMcduId = "3CA2";
    constexpr auto kDisplayPathFormat = "/MCDU/Display/%1";
    constexpr auto kKeyPathFormat = "/MCDU/Key/%1?Key=%2";
    constexpr auto kValueField = "Value";
    constexpr int kTransferTimeoutMs = 2000;
    constexpr int kPollFailuresBeforeUnavailable = 2;
    constexpr int kMaxGlyph = 255;

    QUrl DisplayUrl(const QString& baseUrl)
    {
        return QUrl(baseUrl + QString::fromLatin1(kDisplayPathFormat).arg(QString::fromLatin1(kRightMcduId)));
    }

    QUrl KeyUrl(const QString& baseUrl, const int key)
    {
        return QUrl(baseUrl + QString::fromLatin1(kKeyPathFormat).arg(QString::fromLatin1(kRightMcduId)).arg(key));
    }

    std::optional<int> GlyphOf(const QJsonValue& cell)
    {
        if (!cell.isArray())
        {
            return std::nullopt;
        }

        const QJsonArray parts = cell.toArray();
        if (parts.isEmpty())
        {
            return FslabsMcduScreen::kBlankGlyph;
        }

        const int glyph = parts.at(0).toInt(-1);
        if (glyph < 0 || glyph > kMaxGlyph)
        {
            return std::nullopt;
        }

        return glyph;
    }

    std::optional<FslabsMcduScreen> ScreenIn(QNetworkReply& reply)
    {
        if (reply.error() != QNetworkReply::NoError)
        {
            return std::nullopt;
        }

        return FslabsMcduClient::ParseDisplayResponse(reply.readAll());
    }
}

FslabsMcduClient::FslabsMcduClient()
    : baseUrl_(QString::fromLatin1(kBaseUrl))
{
}

FslabsMcduClient::~FslabsMcduClient() = default;

std::optional<FslabsMcduScreen> FslabsMcduClient::ParseDisplayResponse(const QByteArray& body)
{
    const QJsonArray cells = QJsonDocument::fromJson(body).object().value(QLatin1String(kValueField)).toArray();
    if (cells.size() != FslabsMcduScreen::kCells)
    {
        return std::nullopt;
    }

    FslabsMcduScreen screen;
    for (int index = 0; index < FslabsMcduScreen::kCells; ++index)
    {
        const std::optional<int> glyph = GlyphOf(cells.at(index));
        if (!glyph.has_value())
        {
            return std::nullopt;
        }

        screen.glyphs[static_cast<std::size_t>(index)] = static_cast<std::uint8_t>(*glyph);
    }

    return screen;
}

void FslabsMcduClient::Poll()
{
    if (displayReply_ != nullptr)
    {
        return;
    }

    QNetworkAccessManager& network = EnsureNetwork();
    pressCountWhenPolled_ = pressCount_;
    displayReply_ = network.get(QNetworkRequest(DisplayUrl(baseUrl_)));
    if (displayReply_ == nullptr)
    {
        RegisterPollResult(false);

        return;
    }

    QObject::connect(displayReply_, &QNetworkReply::finished, &network, [this] { OnDisplayFinished(); });
}

void FslabsMcduClient::OnDisplayFinished()
{
    QNetworkReply* reply = displayReply_;
    displayReply_ = nullptr;
    reply->deleteLater();

    const std::optional<FslabsMcduScreen> screen = ScreenIn(*reply);
    if (!screen.has_value())
    {
        RegisterPollResult(false);

        return;
    }

    screen_ = *screen;
    if (pressCountWhenPolled_ == pressCount_)
    {
        ++screenSerial_;
    }

    RegisterPollResult(true);
}

void FslabsMcduClient::RegisterPollResult(const bool succeeded)
{
    if (succeeded)
    {
        if (!available_)
        {
            LOG_INFO("FSLabs remote MCDU reachable: answering at %s", qUtf8Printable(baseUrl_));
        }

        consecutivePollFailures_ = 0;
        available_ = true;

        return;
    }

    ++consecutivePollFailures_;
    if (available_ && consecutivePollFailures_ >= kPollFailuresBeforeUnavailable)
    {
        available_ = false;

        LOG_WARN("FSLabs remote MCDU unreachable: the AOC requests wait until it answers again");
    }
}

void FslabsMcduClient::Press(const int key)
{
    ++pressCount_;

    const QNetworkReply* reply = EnsureNetwork().get(QNetworkRequest(KeyUrl(baseUrl_, key)));
    if (reply != nullptr)
    {
        QObject::connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    }

    ReportPress(key);
}

void FslabsMcduClient::ReportPress(const int key) const
{
    if (!probe::IsOn())
    {
        return;
    }

    probe::Line(probe::Channel::Writes, QStringLiteral("mcdu %1 key=%2")
                .arg(QString::fromLatin1(kRightMcduId))
                .arg(key));
}

bool FslabsMcduClient::IsAvailable() const
{
    return available_;
}

const FslabsMcduScreen& FslabsMcduClient::Screen() const
{
    return screen_;
}

std::uint64_t FslabsMcduClient::ScreenSerial() const
{
    return screenSerial_;
}

QNetworkAccessManager& FslabsMcduClient::EnsureNetwork()
{
    if (network_ == nullptr)
    {
        network_ = std::make_unique<QNetworkAccessManager>();
        network_->setTransferTimeout(kTransferTimeoutMs);
    }

    return *network_;
}

#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSMCDUCLIENT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSMCDUCLIENT_H

#include <cstdint>
#include <memory>
#include <optional>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include "FslabsMcduGateway.h"

class QNetworkAccessManager;
class QNetworkReply;

enum class FslabsMcduSide : std::uint8_t { Left, Right };

class FslabsMcduClient final : public FslabsMcduGateway
{
public:
    explicit FslabsMcduClient(FslabsMcduSide side);
    ~FslabsMcduClient() override;

    void Poll() override;
    [[nodiscard]] bool IsAvailable() const override;
    [[nodiscard]] const FslabsMcduScreen& Screen() const override;
    [[nodiscard]] std::uint64_t ScreenSerial() const override;
    void Press(int key) override;

    void SetEndpointForTest(const QString& baseUrl) { baseUrl_ = baseUrl; }

    [[nodiscard]] static std::optional<FslabsMcduScreen> ParseDisplayResponse(const QByteArray& body);

private:
    QNetworkAccessManager& EnsureNetwork();
    void OnDisplayFinished();
    void RegisterPollResult(bool succeeded);
    void ReportPress(int key) const;

    FslabsMcduSide side_;
    QString baseUrl_;
    std::unique_ptr<QNetworkAccessManager> network_;
    QNetworkReply* displayReply_ = nullptr;
    FslabsMcduScreen screen_;
    std::uint64_t screenSerial_ = 0;
    std::uint64_t pressCount_ = 0;
    std::uint64_t pressCountWhenPolled_ = 0;
    bool available_ = false;
    int consecutivePollFailures_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSMCDUCLIENT_H

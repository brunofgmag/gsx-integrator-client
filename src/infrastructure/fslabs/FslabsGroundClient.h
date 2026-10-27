#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSGROUNDCLIENT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSGROUNDCLIENT_H

#include <memory>
#include <optional>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include "FslabsGroundGateway.h"

class QNetworkAccessManager;
class QNetworkReply;

struct FslabsGroundReading
{
    FslabsHoldState forwardHold;
    FslabsHoldState aftHold;
};

class FslabsGroundClient final : public FslabsGroundGateway
{
public:
    FslabsGroundClient();
    ~FslabsGroundClient() override;

    void Poll() override;
    [[nodiscard]] bool IsAvailable() const override;
    [[nodiscard]] std::optional<FslabsHoldState> HoldState(FslabsHold hold) const override;
    void ToggleHold(FslabsHold hold) override;

    void SetEndpointForTest(const QString& baseUrl)
    {
        readUrl_ = baseUrl;
        commandUrl_ = baseUrl;
    }

    [[nodiscard]] static std::optional<FslabsGroundReading> ParseGroundResponse(const QByteArray& body);

private:
    QNetworkAccessManager& EnsureNetwork();
    void OnGroundFinished();
    void RegisterPollResult(bool succeeded);
    void ReportToggle(FslabsHold hold) const;

    QString readUrl_;
    QString commandUrl_;
    std::unique_ptr<QNetworkAccessManager> network_;
    QNetworkReply* groundReply_ = nullptr;
    std::optional<FslabsGroundReading> reading_;
    bool available_ = false;
    int consecutivePollFailures_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSGROUNDCLIENT_H

#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777TABLETCLIENT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777TABLETCLIENT_H

#include <memory>
#include <string>
#include "Pmdg777TabletGateway.h"

class CommBusBridgeGateway;

class Pmdg777TabletClient final : public Pmdg777TabletGateway
{
public:
    explicit Pmdg777TabletClient(CommBusBridgeGateway* bridge);
    explicit Pmdg777TabletClient(std::unique_ptr<CommBusBridgeGateway> bridge);

    void Poll() override;
    [[nodiscard]] bool IsAvailable() const override;
    [[nodiscard]] bool EfbPlanImported() const override;

    void SendFuelTotalLbs(int lbs) override;
    void SendPaxTotal(int count) override;
    void SendCargoTotalLbs(int lbs) override;
    void RequestGroundConn(const std::string& key) override;

    [[nodiscard]] static std::string BuildWbPayload(const std::string& field, int value);
    [[nodiscard]] static std::string BuildGroundConn(const std::string& key);
    [[nodiscard]] static bool IsSimbriefFetchSuccess(const std::string& json);

private:
    void SendWbPayload(const std::string& field, int value) const;
    void OnInbound(const std::string& payload);

    std::unique_ptr<CommBusBridgeGateway> ownedBridge_;
    CommBusBridgeGateway* bridge_;
    bool bridgeSetup_ = false;
    bool subscribed_ = false;
    bool efbPlanImported_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777TABLETCLIENT_H

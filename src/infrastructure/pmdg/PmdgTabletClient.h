#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGTABLETCLIENT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGTABLETCLIENT_H

#include <map>
#include <set>
#include <memory>
#include <optional>
#include <string>
#include "PmdgTabletGateway.h"

class CommBusBridgeGateway;

class PmdgTabletClient final : public PmdgTabletGateway
{
public:
    explicit PmdgTabletClient(CommBusBridgeGateway* bridge);
    explicit PmdgTabletClient(std::unique_ptr<CommBusBridgeGateway> bridge);
    ~PmdgTabletClient() override;

    void Poll() override;
    [[nodiscard]] bool IsAvailable() const override;
    [[nodiscard]] bool EfbPlanImported() const override;
    [[nodiscard]] std::optional<bool> DoorOpen(const std::string& key) const override;
    [[nodiscard]] bool DoorMoving(const std::string& key) const override;

    void SendFuelTotalLbs(int lbs) override;
    void SendPaxTotal(int count) override;
    void SendCargoTotalLbs(int lbs) override;
    void RequestGroundConn(const std::string& key) override;
    void RequestState() override;

    [[nodiscard]] static std::string BuildWbPayload(const std::string& field, int value);
    [[nodiscard]] static std::string BuildGroundConn(const std::string& key);
    [[nodiscard]] static bool IsSimbriefFetchSuccess(const std::string& json);
    struct DoorSnapshot
    {
        std::map<std::string, bool> settled;
        std::set<std::string> moving;
    };

    [[nodiscard]] static DoorSnapshot ParseDoorStates(const std::string& json);

private:
    void SendWbPayload(const std::string& field, int value) const;
    void OnInbound(const std::string& payload);

    std::unique_ptr<CommBusBridgeGateway> ownedBridge_;
    CommBusBridgeGateway* bridge_;
    bool bridgeSetup_ = false;
    bool subscribed_ = false;
    bool efbPlanImported_ = false;
    std::map<std::string, bool> doorOpen_;
    std::set<std::string> doorMoving_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGTABLETCLIENT_H

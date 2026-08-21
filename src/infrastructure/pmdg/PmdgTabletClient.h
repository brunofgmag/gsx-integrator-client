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
    [[nodiscard]] bool GroundConnMoving(const std::string& key) const override;
    [[nodiscard]] std::optional<bool> PassengerEntryViaJetway() const override;

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
    [[nodiscard]] static std::optional<bool> ParsePassengerEntry(const std::string& json);

private:
    void SendToPlane(const std::string& payload) const;
    void SendWbPayload(const std::string& field, int value) const;
    [[nodiscard]] static std::optional<std::set<std::string>> ParseGroundConnMoving(const std::string& json);
    void OnInbound(const std::string& payload);
    void MaybeProbePress();
    static void ReportProbe(const std::string& payload);

    std::unique_ptr<CommBusBridgeGateway> ownedBridge_;
    CommBusBridgeGateway* bridge_;
    bool bridgeSetup_ = false;
    bool subscribed_ = false;
    bool probePressSent_ = false;
    bool efbPlanImported_ = false;
    std::map<std::string, bool> doorOpen_;
    std::set<std::string> doorMoving_;
    std::set<std::string> groundConnMoving_;
    std::optional<bool> passengerEntryJetway_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGTABLETCLIENT_H

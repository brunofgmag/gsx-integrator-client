#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDGTABLETGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDGTABLETGATEWAY_H

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include "../../src/infrastructure/pmdg/PmdgTabletGateway.h"

class FakePmdgTabletGateway final : public PmdgTabletGateway
{
public:
    bool available = true;
    bool efbPlanImported = false;
    int pollCalls = 0;
    std::vector<int> fuelSends;
    std::vector<int> paxSends;
    std::vector<int> cargoSends;
    std::vector<std::string> groundConnRequests;
    std::map<std::string, bool> doorOpen;
    std::set<std::string> moving;
    std::set<std::string> groundConnMoving;
    int stateRequests = 0;
    std::optional<bool> passengerEntryJetway;
    std::optional<PmdgWeightEcho> weightEcho;
    std::optional<bool> jetwayInhibited;
    std::optional<bool> ownStairsDeployed;
    std::vector<std::string> groundVehicleRequests;

    void Poll() override { ++pollCalls; }
    [[nodiscard]] bool IsAvailable() const override { return available; }
    [[nodiscard]] bool EfbPlanImported() const override { return efbPlanImported; }

    [[nodiscard]] bool DoorMoving(const std::string& key) const override
    {
        return moving.contains(key);
    }

    [[nodiscard]] std::optional<bool> DoorOpen(const std::string& key) const override
    {
        const auto it = doorOpen.find(key);

        return it == doorOpen.end() ? std::nullopt : std::optional(it->second);
    }

    [[nodiscard]] bool GroundConnMoving(const std::string& key) const override
    {
        return groundConnMoving.contains(key);
    }

    [[nodiscard]] std::optional<bool> PassengerEntryViaJetway() const override
    {
        return passengerEntryJetway;
    }

    [[nodiscard]] std::optional<PmdgWeightEcho> LastWeightEcho() const override
    {
        return weightEcho;
    }

    [[nodiscard]] std::optional<bool> JetwayInhibited() const override { return jetwayInhibited; }
    [[nodiscard]] std::optional<bool> OwnStairsDeployed() const override { return ownStairsDeployed; }

    void SendFuelTotalLbs(const int lbs) override { fuelSends.push_back(lbs); }
    void SendPaxTotal(const int count) override { paxSends.push_back(count); }
    void SendCargoTotalLbs(const int lbs) override { cargoSends.push_back(lbs); }
    void RequestGroundConn(const std::string& key) override { groundConnRequests.push_back(key); }
    void RequestGroundVehicle(const std::string& key) override { groundVehicleRequests.push_back(key); }
    void RequestState() override { ++stateRequests; }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDGTABLETGATEWAY_H

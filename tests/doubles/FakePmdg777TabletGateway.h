#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDG777TABLETGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDG777TABLETGATEWAY_H

#include <string>
#include <vector>
#include "../../src/infrastructure/pmdg/Pmdg777TabletGateway.h"

class FakePmdg777TabletGateway final : public Pmdg777TabletGateway
{
public:
    bool available = true;
    bool efbPlanImported = false;
    int pollCalls = 0;
    std::vector<int> fuelSends;
    std::vector<int> paxSends;
    std::vector<int> cargoSends;
    std::vector<std::string> groundConnRequests;

    void Poll() override { ++pollCalls; }
    [[nodiscard]] bool IsAvailable() const override { return available; }
    [[nodiscard]] bool EfbPlanImported() const override { return efbPlanImported; }

    void SendFuelTotalLbs(const int lbs) override { fuelSends.push_back(lbs); }
    void SendPaxTotal(const int count) override { paxSends.push_back(count); }
    void SendCargoTotalLbs(const int lbs) override { cargoSends.push_back(lbs); }
    void RequestGroundConn(const std::string& key) override { groundConnRequests.push_back(key); }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEPMDG777TABLETGATEWAY_H

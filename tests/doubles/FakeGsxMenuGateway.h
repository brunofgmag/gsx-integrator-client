#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXMENUGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXMENUGATEWAY_H

#include "../../src/domain/ports/GsxMenuGateway.h"

class FakeGsxMenuGateway final : public GsxMenuGateway
{
public:
    bool callJetwayResult = true;
    bool callStairsResult = true;
    bool confirmGoodEnginesResult = true;

    int callJetwayCalls = 0;
    int callStairsCalls = 0;
    int repositionCalls = 0;
    int simbriefLoadCalls = 0;
    int boardingCalls = 0;
    int deboardingCalls = 0;
    int pushbackCalls = 0;
    int refuelingCalls = 0;
    int confirmGoodEnginesCalls = 0;

    [[nodiscard]] bool CallJetway() override
    {
        ++callJetwayCalls;
        return callJetwayResult;
    }

    [[nodiscard]] bool CallStairs() override
    {
        ++callStairsCalls;
        return callStairsResult;
    }

    [[nodiscard]] bool RepositionAircraft() override
    {
        ++repositionCalls;
        return true;
    }

    [[nodiscard]] bool RequestSimbriefLoad() override
    {
        ++simbriefLoadCalls;
        return true;
    }

    [[nodiscard]] bool RequestBoarding() override
    {
        ++boardingCalls;
        return true;
    }

    [[nodiscard]] bool RequestDeboarding() override
    {
        ++deboardingCalls;
        return true;
    }

    [[nodiscard]] bool RequestPushback() override
    {
        ++pushbackCalls;
        return true;
    }

    [[nodiscard]] bool RequestRefueling() override
    {
        ++refuelingCalls;
        return true;
    }

    [[nodiscard]] bool ConfirmGoodEngines() override
    {
        ++confirmGoodEnginesCalls;
        return confirmGoodEnginesResult;
    }

    void DisableGsxMenu() override {}
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXMENUGATEWAY_H

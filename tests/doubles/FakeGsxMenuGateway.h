#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXMENUGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXMENUGATEWAY_H

#include "../../src/domain/ports/GsxMenuGateway.h"

class FakeGsxMenuGateway final : public GsxMenuGateway
{
public:
    bool callJetwayResult = true;
    bool callStairsResult = true;
    bool confirmGoodEnginesResult = true;
    bool completePushbackResult = true;
    bool completeRefuelResult = true;

    int callJetwayCalls = 0;
    int callStairsCalls = 0;
    int repositionCalls = 0;
    int simbriefLoadCalls = 0;
    int boardingCalls = 0;
    int deboardingCalls = 0;
    int pushbackCalls = 0;
    int refuelingCalls = 0;
    int confirmGoodEnginesCalls = 0;
    int completePushbackCalls = 0;
    int completeRefuelCalls = 0;
    int toggleGpuCalls = 0;
    int requestCateringCalls = 0;
    int requestLavatoryCalls = 0;
    int requestWaterCalls = 0;
    int requestCleaningCalls = 0;

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

    [[nodiscard]] bool CompletePushback() override
    {
        ++completePushbackCalls;
        return completePushbackResult;
    }

    [[nodiscard]] bool CompleteRefuel() override
    {
        ++completeRefuelCalls;
        return completeRefuelResult;
    }

    [[nodiscard]] bool ToggleGpu() override
    {
        ++toggleGpuCalls;
        return true;
    }

    [[nodiscard]] bool RequestCatering() override
    {
        ++requestCateringCalls;
        return true;
    }

    [[nodiscard]] bool RequestLavatory() override
    {
        ++requestLavatoryCalls;
        return true;
    }

    [[nodiscard]] bool RequestWater() override
    {
        ++requestWaterCalls;
        return true;
    }

    [[nodiscard]] bool RequestCleaning() override
    {
        ++requestCleaningCalls;
        return true;
    }

    void DisableGsxMenu() override {}
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXMENUGATEWAY_H

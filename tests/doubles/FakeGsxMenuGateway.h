#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXMENUGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXMENUGATEWAY_H

#include "../../src/domain/ports/GsxMenuGateway.h"

class FakeGsxMenuGateway final : public GsxMenuGateway
{
public:
    bool confirmGoodEnginesResult = true;
    bool completePushbackResult = true;
    bool stairsKeptForPassengers = false;

    int callJetwayCalls = 0;
    int callStairsCalls = 0;
    int repositionCalls = 0;
    int simbriefLoadCalls = 0;
    int boardingCalls = 0;
    int deboardingCalls = 0;
    int pushbackCalls = 0;
    int departureClearanceCalls = 0;
    int openPushbackPanelCalls = 0;
    int refuelingCalls = 0;
    int confirmGoodEnginesCalls = 0;
    int completePushbackCalls = 0;
    int completeRefuelCalls = 0;
    int completeBoardingCalls = 0;
    int toggleGpuCalls = 0;
    int requestCateringCalls = 0;
    int requestLavatoryCalls = 0;
    int requestWaterCalls = 0;
    int requestCleaningCalls = 0;
    int turnaroundTurnedCalls = 0;
    int pushbackStartedCalls = 0;

    void CallJetway() override { ++callJetwayCalls; }

    void CallStairs() override { ++callStairsCalls; }

    void RepositionAircraft() override { ++repositionCalls; }

    void RequestSimbriefLoad() override { ++simbriefLoadCalls; }

    void RequestBoarding() override { ++boardingCalls; }

    void RequestDeboarding() override { ++deboardingCalls; }

    void RequestPushback() override { ++pushbackCalls; }

    void RequestDepartureClearance() override { ++departureClearanceCalls; }

    void OpenPushbackPanel() override { ++openPushbackPanelCalls; }

    void RequestRefueling() override { ++refuelingCalls; }

    void CompleteRefuel() override { ++completeRefuelCalls; }

    void CompleteBoarding() override { ++completeBoardingCalls; }

    void ToggleGpu() override { ++toggleGpuCalls; }

    void RequestCatering() override { ++requestCateringCalls; }

    void RequestLavatory() override { ++requestLavatoryCalls; }

    void RequestWater() override { ++requestWaterCalls; }

    void RequestCleaning() override { ++requestCleaningCalls; }

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

    [[nodiscard]] bool WereStairsKeptForPassengers() const override { return stairsKeptForPassengers; }

    void DisableGsxMenu() override {}

    void OnTurnaroundTurned() override { ++turnaroundTurnedCalls; }
    void OnPushbackStarted() override { ++pushbackStartedCalls; }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXMENUGATEWAY_H

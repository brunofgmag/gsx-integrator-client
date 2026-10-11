#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXMENUGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXMENUGATEWAY_H

#include <cstddef>
#include <vector>
#include "../../src/domain/ports/GsxMenuGateway.h"

enum class RequestKind : std::size_t
{
    CallJetway,
    CallStairs,
    Reposition,
    SimbriefLoad,
    Boarding,
    Deboarding,
    Pushback,
    DepartureClearance,
    Refueling,
    ConfirmGoodEngines,
    CompletePushback,
    CompleteRefuel,
    CompleteBoarding,
    ToggleGpu,
    Catering,
    Lavatory,
    Water,
    Cleaning,
    Count,
};

class FakeGsxMenuGateway final : public GsxMenuGateway
{
public:
    bool confirmGoodEnginesResult = true;
    bool completePushbackResult = true;
    bool stairsKeptInPlace = false;

    int callJetwayCalls = 0;
    int callStairsCalls = 0;
    int repositionCalls = 0;
    int simbriefLoadCalls = 0;
    int boardingCalls = 0;
    int deboardingCalls = 0;
    int pushbackCalls = 0;
    int departureClearanceCalls = 0;
    int openPushbackPanelCalls = 0;
    int closePushbackPanelCalls = 0;
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
    std::vector<RequestKind> requestLog;

    void CallJetway() override
    {
        ++callJetwayCalls;
        requestLog.push_back(RequestKind::CallJetway);
    }

    void CallStairs() override
    {
        ++callStairsCalls;
        requestLog.push_back(RequestKind::CallStairs);
    }

    void RepositionAircraft() override
    {
        ++repositionCalls;
        requestLog.push_back(RequestKind::Reposition);
    }

    void RequestSimbriefLoad() override
    {
        ++simbriefLoadCalls;
        requestLog.push_back(RequestKind::SimbriefLoad);
    }

    void RequestBoarding() override
    {
        ++boardingCalls;
        requestLog.push_back(RequestKind::Boarding);
    }

    void RequestDeboarding() override
    {
        ++deboardingCalls;
        requestLog.push_back(RequestKind::Deboarding);
    }

    void RequestPushback() override
    {
        ++pushbackCalls;
        requestLog.push_back(RequestKind::Pushback);
    }

    void RequestDepartureClearance() override
    {
        ++departureClearanceCalls;
        requestLog.push_back(RequestKind::DepartureClearance);
    }

    void OpenPushbackPanel() override { ++openPushbackPanelCalls; }
    void ClosePushbackPanel() override { ++closePushbackPanelCalls; }

    void RequestRefueling() override
    {
        ++refuelingCalls;
        requestLog.push_back(RequestKind::Refueling);
    }

    void CompleteRefuel() override
    {
        ++completeRefuelCalls;
        requestLog.push_back(RequestKind::CompleteRefuel);
    }

    void CompleteBoarding() override
    {
        ++completeBoardingCalls;
        requestLog.push_back(RequestKind::CompleteBoarding);
    }

    void ToggleGpu() override
    {
        ++toggleGpuCalls;
        requestLog.push_back(RequestKind::ToggleGpu);
    }

    void RequestCatering() override
    {
        ++requestCateringCalls;
        requestLog.push_back(RequestKind::Catering);
    }

    void RequestLavatory() override
    {
        ++requestLavatoryCalls;
        requestLog.push_back(RequestKind::Lavatory);
    }

    void RequestWater() override
    {
        ++requestWaterCalls;
        requestLog.push_back(RequestKind::Water);
    }

    void RequestCleaning() override
    {
        ++requestCleaningCalls;
        requestLog.push_back(RequestKind::Cleaning);
    }

    [[nodiscard]] bool ConfirmGoodEngines() override
    {
        ++confirmGoodEnginesCalls;
        requestLog.push_back(RequestKind::ConfirmGoodEngines);

        return confirmGoodEnginesResult;
    }

    [[nodiscard]] bool CompletePushback() override
    {
        ++completePushbackCalls;
        requestLog.push_back(RequestKind::CompletePushback);

        return completePushbackResult;
    }

    [[nodiscard]] bool WereStairsKeptInPlace() const override { return stairsKeptInPlace; }

    void DisableGsxMenu() override {}

    void OnTurnaroundTurned() override { ++turnaroundTurnedCalls; }
    void OnPushbackStarted() override { ++pushbackStartedCalls; }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXMENUGATEWAY_H

#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXSERVICE_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXSERVICE_H

#include "../../src/domain/ports/GsxGateway.h"

class FakeGsxService final : public GsxGateway
{
public:
    GsxStateStatus refuelingState = GsxStateStatus::Unavailable;
    GsxStateStatus boardingState = GsxStateStatus::Unavailable;
    GsxStateStatus departureState = GsxStateStatus::Unavailable;
    bool hoseConnected = false;
    double refuelCounterGallons = 0.0;
    bool waitingForEngines = false;
    bool pushbackStarted = false;
    bool pushbackFinished = false;
    bool repositioning = false;
    bool stairsInPlace = false;
    bool jetwayInPlace = false;
    bool stairsAvailable = false;
    bool jetwayAvailable = false;
    GsxStateStatus deboardingState = GsxStateStatus::Unavailable;
    int plannedPassengers = 0;
    int boardedPassengers = 0;
    int deboardedPassengers = 0;
    double cargoPercent = 0.0;
    double deboardingCargoPercent = 0.0;
    bool refuelingCompleted = false;
    bool boardingCompleted = false;
    bool pushbackCompleted = false;
    bool deboardingCompleted = false;
    bool simbriefLoaded = false;
    bool onGround = true;
    bool goodEngineStartConfirmation = false;
    int takeOverCalls = 0;

    [[nodiscard]] bool IsAvailable() const override { return true; }

    [[nodiscard]] GsxStateStatus GetStateStatus(const GsxState state) override
    {
        switch (state)
        {
        case GsxState::Refueling:
            return refuelingState;
        case GsxState::Boarding:
            return boardingState;
        case GsxState::Pushback:
            return departureState;
        case GsxState::Deboarding:
            return deboardingState;
        default:
            return GsxStateStatus::Unavailable;
        }
    }

    [[nodiscard]] bool WasStateCompleted(const GsxState state) const override
    {
        switch (state)
        {
        case GsxState::Refueling:
            return refuelingCompleted;
        case GsxState::Boarding:
            return boardingCompleted;
        case GsxState::Pushback:
            return pushbackCompleted;
        case GsxState::Deboarding:
            return deboardingCompleted;
        default:
            return false;
        }
    }

    [[nodiscard]] bool IsWaitingForEngines() const override { return waitingForEngines; }
    [[nodiscard]] bool IsFuelHoseConnected() const override { return hoseConnected; }
    [[nodiscard]] double GetRefuelCounterGallons() const override { return refuelCounterGallons; }
    [[nodiscard]] bool HasPushbackStarted() const override { return pushbackStarted; }
    [[nodiscard]] bool IsPushbackFinished() const override { return pushbackFinished; }
    [[nodiscard]] bool IsRepositioning() const override { return repositioning; }
    [[nodiscard]] int GetPlannedPassengers() const override { return plannedPassengers; }
    [[nodiscard]] int GetBoardedPassengers() override { return boardedPassengers; }
    [[nodiscard]] int GetDeboardedPassengers() override { return deboardedPassengers; }
    [[nodiscard]] double GetBoardingCargoPercent() const override { return cargoPercent; }
    [[nodiscard]] double GetDeboardingCargoPercent() const override { return deboardingCargoPercent; }
    [[nodiscard]] bool AreStairsInPlace() const override { return stairsInPlace; }
    [[nodiscard]] bool IsJetwayInPlace() const override { return jetwayInPlace; }
    [[nodiscard]] bool AreStairsAvailable() const override { return stairsAvailable; }
    [[nodiscard]] bool IsJetwayAvailable() const override { return jetwayAvailable; }
    [[nodiscard]] bool IsSimbriefLoaded() const override { return simbriefLoaded; }
    [[nodiscard]] bool IsAircraftOnGround() const override { return onGround; }
    [[nodiscard]] bool IsGoodEngineStartConfirmationEnabled() const override { return goodEngineStartConfirmation; }

    void TakeOverFuelAndPayload() override
    {
        ++takeOverCalls;
    }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEGSXSERVICE_H

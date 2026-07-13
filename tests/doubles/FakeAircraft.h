#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEAIRCRAFT_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEAIRCRAFT_H

#include "../../src/domain/ports/Aircraft.h"

class FakeAircraft final : public Aircraft
{
public:
    bool cargo = false;
    bool flightPlanLoaded = false;
    double plannedFuelKg = 0.0;
    double plannedZfwKg = 0.0;
    double emptyZfwKg = 0.0;
    int plannedPax = 0;
    double currentFuelKg = 0.0;
    double currentZfwKg = 0.0;
    bool smartSwitchActivated = false;
    bool powered = false;
    bool readyToPush = false;
    bool readyToDeboard = false;
    bool engineRunning = false;
    bool parkingBrakeSet = false;
    bool supportsStairsOrJetways = true;
    bool completesPushbackViaInterruptMenu = false;
    RefuelBy refuelMethod = RefuelBy::Self;
    BoardBy boardMethod = BoardBy::Self;
    int consumeSmartSwitchCalls = 0;
    int onLoadingStartedCalls = 0;

    [[nodiscard]] const char* GetName() const override { return "Fake Aircraft"; }
    [[nodiscard]] bool IsCargoVariant() const override { return cargo; }

    void OnLoadingStarted() override
    {
        ++onLoadingStartedCalls;
    }

    [[nodiscard]] bool IsFlightPlanLoaded() const override { return flightPlanLoaded; }
    [[nodiscard]] double GetPlannedFuelKg() const override { return plannedFuelKg; }
    [[nodiscard]] double GetPlannedZfwKg() const override { return plannedZfwKg; }
    [[nodiscard]] double GetEmptyZfwKg() const override { return emptyZfwKg; }
    [[nodiscard]] int GetPlannedPassengers() const override { return plannedPax; }
    [[nodiscard]] double GetCurrentFuelKg() const override { return currentFuelKg; }
    void SetCurrentFuelKg(const double value) override { currentFuelKg = value; }
    [[nodiscard]] double GetCurrentZfwKg() const override { return currentZfwKg; }
    void SetCurrentZfwKg(const double value) override { currentZfwKg = value; }
    [[nodiscard]] bool SupportsStairsOrJetways() const override { return supportsStairsOrJetways; }
    [[nodiscard]] bool CompletesPushbackViaInterruptMenu() const override { return completesPushbackViaInterruptMenu; }
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return refuelMethod; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return boardMethod; }

    [[nodiscard]] bool ConsumeSmartSwitch() override
    {
        ++consumeSmartSwitchCalls;
        return smartSwitchActivated;
    }

    [[nodiscard]] bool IsPowered() const override { return powered; }
    [[nodiscard]] bool IsReadyToPush() const override { return readyToPush; }
    [[nodiscard]] bool IsReadyToDeboard() const override { return readyToDeboard; }
    [[nodiscard]] bool IsEngineRunning() const override { return engineRunning; }
    [[nodiscard]] bool IsParkingBrakeSet() const override { return parkingBrakeSet; }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEAIRCRAFT_H

#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEAIRCRAFT_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEAIRCRAFT_H

#include "../../src/domain/ports/Aircraft.h"

class FakeAircraft final : public Aircraft
{
public:
    bool cargo = false;
    bool flightPlanLoaded = false;
    bool flightPlanDiffersFromTheOfp = false;
    double plannedFuelKg = 0.0;
    double plannedZfwKg = 0.0;
    double emptyZfwKg = 0.0;
    double plannedOperatingEmptyKg = 0.0;
    double crewOnBoardKg = 0.0;
    int plannedPax = 0;
    double currentFuelKg = 0.0;
    double currentZfwKg = 0.0;
    double fuelCapacityKg = 0.0;
    int fuelCapacityReadsBeforeArrival = 0;
    mutable int fuelCapacityReads = 0;
    bool smartSwitchActivated = false;
    bool powered = false;
    bool readyToPush = false;
    bool readyToDeboard = false;
    bool engineRunning = false;
    bool heldInPlace = false;
    bool parkingBrakeSet = false;
    DoorStatus doorStatus = DoorStatus::Unknown;
    bool supportsStairsOrJetways = true;
    bool carriesItsOwnStairs = false;
    std::vector<AircraftRule*> rules;
    bool completesPushbackViaInterruptMenu = false;
    std::optional<GroundPowerStatus> groundPowerStatus = std::nullopt;
    bool supportsChocksControl = false;
    bool chocksPlaced = false;
    int setChocksCalls = 0;
    bool supportsGroundPowerControl = false;
    bool requiresEfbFlightPlan = false;
    bool groundPowerOn = false;
    int setGroundPowerCalls = 0;
    int closeAllDoorsCalls = 0;
    bool doorsHeldClosed = false;
    int holdDoorsClosedCalls = 0;
    int clearOwnGroundEquipmentCalls = 0;
    RefuelBy refuelMethod = RefuelBy::Self;
    BoardBy boardMethod = BoardBy::Self;
    int consumeSmartSwitchCalls = 0;
    int onLoadingStartedCalls = 0;

    [[nodiscard]] bool IsCargoVariant() const override { return cargo; }

    void OnLoadingStarted() override
    {
        ++onLoadingStartedCalls;
    }

    [[nodiscard]] bool RequiresEfbFlightPlan() const override { return requiresEfbFlightPlan; }
    [[nodiscard]] bool IsFlightPlanLoaded() const override { return flightPlanLoaded; }
    [[nodiscard]] bool FlightPlanDiffersFromTheOfp() const override { return flightPlanDiffersFromTheOfp; }
    [[nodiscard]] double GetPlannedFuelKg() const override { return plannedFuelKg; }
    [[nodiscard]] double GetPlannedZfwKg() const override { return plannedZfwKg; }
    [[nodiscard]] double GetEmptyZfwKg() const override { return emptyZfwKg; }
    [[nodiscard]] double GetPlannedOperatingEmptyKg() const override { return plannedOperatingEmptyKg; }
    [[nodiscard]] double GetCrewOnBoardKg() const override { return crewOnBoardKg; }
    [[nodiscard]] int GetPlannedPassengers() const override { return plannedPax; }
    [[nodiscard]] double GetCurrentFuelKg() const override { return currentFuelKg; }
    [[nodiscard]] double GetFuelCapacityKg() const override
    {
        ++fuelCapacityReads;

        return fuelCapacityReads > fuelCapacityReadsBeforeArrival ? fuelCapacityKg : 0.0;
    }

    void SetCurrentFuelKg(const double value) override { currentFuelKg = value; }
    [[nodiscard]] double GetCurrentZfwKg() const override { return currentZfwKg; }
    void SetCurrentZfwKg(const double value) override { currentZfwKg = value; }
    [[nodiscard]] bool SupportsStairsOrJetways() const override { return supportsStairsOrJetways; }
    [[nodiscard]] bool CarriesItsOwnStairs() const override { return carriesItsOwnStairs; }
    [[nodiscard]] const std::vector<AircraftRule*>& Rules() const override { return rules; }
    [[nodiscard]] bool CompletesPushbackViaInterruptMenu() const override { return completesPushbackViaInterruptMenu; }
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return refuelMethod; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return boardMethod; }

    [[nodiscard]] bool ConsumeSmartSwitch() override
    {
        ++consumeSmartSwitchCalls;
        return smartSwitchActivated;
    }

    [[nodiscard]] bool IsPowered() const override { return powered; }
    [[nodiscard]] std::optional<GroundPowerStatus> GetGroundPowerStatus() const override { return groundPowerStatus; }

    [[nodiscard]] bool SupportsChocksControl() const override { return supportsChocksControl; }

    bool SetChocks(const bool placed) override
    {
        if (!supportsChocksControl)
        {
            return false;
        }

        ++setChocksCalls;
        chocksPlaced = placed;

        return true;
    }

    [[nodiscard]] bool SupportsGroundPowerControl() const override { return supportsGroundPowerControl; }

    void SetGroundPower(const bool on) override
    {
        ++setGroundPowerCalls;
        groundPowerOn = on;
    }

    void CloseAllDoors() override { ++closeAllDoorsCalls; }

    void HoldDoorsClosed(const bool hold) override
    {
        ++holdDoorsClosedCalls;
        doorsHeldClosed = hold;
    }

    void ClearOwnGroundEquipment() override { ++clearOwnGroundEquipmentCalls; }
    [[nodiscard]] DoorStatus GetDoorStatus() const override { return doorStatus; }
    [[nodiscard]] bool IsReadyToPush() const override { return readyToPush; }
    [[nodiscard]] bool IsReadyToDeboard() const override { return readyToDeboard; }
    [[nodiscard]] bool IsEngineRunning() const override { return engineRunning; }
    [[nodiscard]] bool IsHeldInPlace() const override { return heldInPlace || parkingBrakeSet; }
    [[nodiscard]] bool IsParkingBrakeSet() const override { return parkingBrakeSet; }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEAIRCRAFT_H

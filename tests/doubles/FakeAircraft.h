#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEAIRCRAFT_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEAIRCRAFT_H

#include <string>
#include <vector>
#include "../../src/domain/model/MemoryBag.h"
#include "../../src/domain/ports/Aircraft.h"
#include "../../src/domain/turnaround/TurnaroundFacts.h"

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
    bool passengerDoorsHeldClosed = false;
    int holdPassengerDoorsClosedCalls = 0;
    int clearOwnGroundEquipmentCalls = 0;
    RefuelBy refuelMethod = RefuelBy::Self;
    bool supportsFuelTopUp = false;
    bool ignoresFuelWrites = false;
    int setCurrentFuelCalls = 0;
    BoardBy boardMethod = BoardBy::Self;
    int consumeSmartSwitchCalls = 0;
    int onLoadingStartedCalls = 0;
    bool reachable = true;
    MemoryBag memoryToReturn;
    int onTurnaroundStartedCalls = 0;
    int onTurnaroundResumedCalls = 0;
    TurnaroundFacts resumedFacts;
    MemoryBag resumedMemory;
    std::vector<double> fuelWrites;
    std::vector<double> zfwWrites;
    std::vector<std::string> callLog;

    [[nodiscard]] bool IsCargoVariant() const override { return cargo; }
    [[nodiscard]] bool IsReachable() const override { return reachable; }
    [[nodiscard]] MemoryBag TurnaroundMemory() const override { return memoryToReturn; }

    void OnTurnaroundStarted() override
    {
        ++onTurnaroundStartedCalls;
        callLog.emplace_back("OnTurnaroundStarted");
    }

    void OnTurnaroundResumed(const TurnaroundFacts& facts, const MemoryBag& memory) override
    {
        ++onTurnaroundResumedCalls;
        resumedFacts = facts;
        resumedMemory = memory;
        callLog.emplace_back("OnTurnaroundResumed");
    }

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

    void SetCurrentFuelKg(const double value) override
    {
        ++setCurrentFuelCalls;
        fuelWrites.push_back(value);
        if (!ignoresFuelWrites)
        {
            currentFuelKg = value;
        }
    }

    [[nodiscard]] bool SupportsFuelTopUp() const override { return supportsFuelTopUp; }
    [[nodiscard]] double GetCurrentZfwKg() const override { return currentZfwKg; }
    void SetCurrentZfwKg(const double value) override
    {
        zfwWrites.push_back(value);
        currentZfwKg = value;
    }

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
        callLog.emplace_back(placed ? "SetChocks(true)" : "SetChocks(false)");

        return true;
    }

    [[nodiscard]] bool SupportsGroundPowerControl() const override { return supportsGroundPowerControl; }

    void SetGroundPower(const bool on) override
    {
        ++setGroundPowerCalls;
        groundPowerOn = on;
    }

    void CloseAllDoors() override
    {
        ++closeAllDoorsCalls;
        callLog.emplace_back("CloseAllDoors");
    }

    void HoldDoorsClosed(const bool hold) override
    {
        ++holdDoorsClosedCalls;
        doorsHeldClosed = hold;
        if (!hold)
        {
            passengerDoorsHeldClosed = false;
        }

        callLog.emplace_back(hold ? "HoldDoorsClosed(true)" : "HoldDoorsClosed(false)");
    }

    void HoldPassengerDoorsClosed(const bool hold) override
    {
        ++holdPassengerDoorsClosedCalls;
        passengerDoorsHeldClosed = hold;
        callLog.emplace_back(hold ? "HoldPassengerDoorsClosed(true)" : "HoldPassengerDoorsClosed(false)");
    }

    void ClearOwnGroundEquipment() override
    {
        ++clearOwnGroundEquipmentCalls;
        callLog.emplace_back("ClearOwnGroundEquipment");
    }

    [[nodiscard]] DoorStatus GetDoorStatus() const override { return doorStatus; }
    [[nodiscard]] bool IsReadyToPush() const override { return readyToPush; }
    [[nodiscard]] bool IsReadyToDeboard() const override { return readyToDeboard; }
    [[nodiscard]] bool IsEngineRunning() const override { return engineRunning; }
    [[nodiscard]] bool IsHeldInPlace() const override { return heldInPlace || parkingBrakeSet; }
    [[nodiscard]] bool IsParkingBrakeSet() const override { return parkingBrakeSet; }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEAIRCRAFT_H

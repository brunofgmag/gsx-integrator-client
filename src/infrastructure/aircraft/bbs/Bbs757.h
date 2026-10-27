#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_BBS757_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_BBS757_H

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "../SmartSwitch.h"
#include "rules/Bbs757DoorsFollowGsxRule.h"
#include "../../gsx/GsxDoorSync.h"
#include "../../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

class Bbs757 final : public Aircraft
{
public:
    static constexpr auto kName = "BlueBird 757-200";

    Bbs757(VariableGateway* variableGateway, const AutomationStatus* status);

    [[nodiscard]] bool IsCargoVariant() const override;

    void Observe() override;
    [[nodiscard]] static const char* DoorSwitch(GsxDoor door);
    [[nodiscard]] const std::vector<AircraftRule*>& Rules() const override;
    [[nodiscard]] MemoryBag TurnaroundMemory() const override;

    void OnLoadingStarted() override {}

    [[nodiscard]] bool IsReachable() const override;
    void OnTurnaroundStarted() override;
    void OnTurnaroundResumed(const TurnaroundFacts& facts, const MemoryBag& memory) override;

    [[nodiscard]] bool IsFlightPlanLoaded() const override;
    [[nodiscard]] double GetPlannedFuelKg() const override;
    [[nodiscard]] double GetPlannedZfwKg() const override;
    [[nodiscard]] int GetPlannedPassengers() const override;
    [[nodiscard]] double GetEmptyZfwKg() const override;
    [[nodiscard]] double GetPlannedOperatingEmptyKg() const override;
    [[nodiscard]] double GetCrewOnBoardKg() const override;
    [[nodiscard]] std::optional<WeightUnit> GetNativeWeightUnit() const override { return WeightUnit::Lb; }

    [[nodiscard]] double GetCurrentFuelKg() const override;
    [[nodiscard]] double GetFuelCapacityKg() const override;
    void SetCurrentFuelKg(double fuelKg) override;
    [[nodiscard]] double GetCurrentZfwKg() const override;
    void SetCurrentZfwKg(double zfwKg) override;

    [[nodiscard]] bool SupportsStairsOrJetways() const override { return true; }
    [[nodiscard]] bool CompletesPushbackViaInterruptMenu() const override { return false; }
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return RefuelBy::Client; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return BoardBy::Client; }

    [[nodiscard]] bool ConsumeSmartSwitch() override;

    [[nodiscard]] std::optional<GroundPowerStatus> GetGroundPowerStatus() const override;
    [[nodiscard]] bool IsOwnGroundEquipmentAttached() const override;
    void CloseAllDoors() override;
    void HoldDoorsClosed(bool hold) override;
    void HoldPassengerDoorsClosed(bool hold) override;

    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] DoorStatus GetDoorStatus() const override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsHeldInPlace() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    struct ResumedPlan
    {
        double aboveEmptyKg;
        int passengers;
    };

    struct PlannedLoad
    {
        double payloadKg;
        double passengerShare;
    };

    void ReportProbe() const;
    [[nodiscard]] bool IsBeaconOn() const;
    [[nodiscard]] bool AreChocksSet() const;
    [[nodiscard]] PlannedLoad PlanAboard() const;
    [[nodiscard]] PlannedLoad PlanFromLoadOnBoard() const;
    void WriteWhenChanged(const char* lVar, double value);

    VariableGateway* variableGateway_;
    const AutomationStatus* status_;
    SmartSwitch smartSwitch_;
    GsxDoorSync doors_;
    Bbs757DoorsFollowGsxRule doorRule_;
    std::vector<AircraftRule*> rules_;
    std::map<std::string, double> lastWritten_;
    std::optional<ResumedPlan> resumedPlan_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_BBS757_H

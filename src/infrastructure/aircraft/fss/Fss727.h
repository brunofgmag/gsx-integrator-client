#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727_H

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include "../SmartSwitch.h"
#include "rules/Fss727FrontEntryServesTheGroundAccessRule.h"
#include "rules/Fss727HoldsCloseOnceTheirLoaderLeavesRule.h"
#include "rules/Fss727KeepVendorGsxAutomodeOffRule.h"
#include "rules/Fss727MainDeckClosesByTheCargoPanelRule.h"
#include "../../gsx/GsxDoorSync.h"
#include "../../../domain/ports/Aircraft.h"

class GsxGateway;
class VariableGateway;
struct AutomationStatus;

struct Fss727DoorPoint
{
    const char* position;
    int movingLimitTicks;
};

class Fss727 final : public Aircraft
{
public:
    static constexpr auto kName200F = "FSS Boeing 727-200F";
    static constexpr auto kName200ReFreighter = "FSS Boeing 727-200RE Freighter";

    Fss727(VariableGateway* variableGateway, const AutomationStatus* status, const char* name,
           const GsxGateway* gsxGateway = nullptr);

    [[nodiscard]] bool IsCargoVariant() const override;

    void Observe() override;
    [[nodiscard]] const std::vector<AircraftRule*>& Rules() const override;
    void OnLoadingStarted() override {}

    [[nodiscard]] bool IsFlightPlanLoaded() const override;
    [[nodiscard]] double GetPlannedFuelKg() const override;
    [[nodiscard]] double GetPlannedZfwKg() const override;
    [[nodiscard]] int GetPlannedPassengers() const override;
    [[nodiscard]] double GetEmptyZfwKg() const override;
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

    [[nodiscard]] bool SupportsGroundPowerControl() const override { return true; }
    [[nodiscard]] std::optional<GroundPowerStatus> GetGroundPowerStatus() const override;
    void SetGroundPower(bool on) override;
    void CloseAllDoors() override;
    void HoldDoorsClosed(bool hold) override;
    [[nodiscard]] bool IsHeldForDeparture() const;
    [[nodiscard]] int MainDeckCloseRequests() const;
    [[nodiscard]] int HoldCloseRequests() const;
    [[nodiscard]] std::optional<bool> IsMainDeckClosed() const;
    [[nodiscard]] bool SupportsChocksControl() const override { return true; }
    bool SetChocks(bool placed) override;
    void ClearOwnGroundEquipment() override;

    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] DoorStatus GetDoorStatus() const override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsHeldInPlace() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    [[nodiscard]] bool IsBeaconOn() const;
    [[nodiscard]] bool AreChocksSet() const;
    [[nodiscard]] std::optional<double> DoorPointPosition(std::size_t point) const;
    [[nodiscard]] std::optional<bool> DoorOpenAt(std::size_t point) const;

    VariableGateway* variableGateway_;
    const AutomationStatus* status_;
    SmartSwitch smartSwitch_;
    std::span<const Fss727DoorPoint> doorPoints_;
    std::vector<int> movingTicks_;
    GsxDoorSync doors_;
    bool heldForDeparture_ = false;
    int mainDeckCloseRequests_ = 0;
    int holdCloseRequests_ = 0;
    double lastFuelKg_ = -1.0;
    double lastZfwKg_ = -1.0;
    Fss727KeepVendorGsxAutomodeOffRule automodeRule_;
    Fss727FrontEntryServesTheGroundAccessRule frontEntryRule_;
    Fss727HoldsCloseOnceTheirLoaderLeavesRule holdsRule_;
    Fss727MainDeckClosesByTheCargoPanelRule mainDeckRule_;
    std::vector<AircraftRule*> rules_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSS727_H

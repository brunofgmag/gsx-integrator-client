#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGAIRCRAFT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGAIRCRAFT_H

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "SmartSwitch.h"
#include "../gsx/GsxDoorSync.h"
#include "../pmdg/PmdgDoorReconciler.h"
#include "../pmdg/PmdgDoorSource.h"
#include "../pmdg/PmdgGroundConnReconciler.h"
#include "../pmdg/PmdgGroundSource.h"
#include "../pmdg/PmdgPayloadWriter.h"
#include "../pmdg/PmdgRouteFile.h"
#include "../pmdg/PmdgTabletGateway.h"
#include "../../domain/ports/Aircraft.h"

class PmdgDataGateway;
class VariableGateway;
struct AutomationStatus;

struct PmdgAircraftSpec
{
    int doorSlots = 0;
    int mainDeckDoorSlot = -1;
    bool cargoVariant = false;
    DoorBaseline doorBaseline = DoorBaseline::Unknown;
    std::vector<std::string> smartSwitchLVars;
    SmartSwitch::Predicate smartSwitchPressed;
};

class PmdgAircraft : public Aircraft, protected PmdgDoorSource, protected PmdgGroundSource
{
public:
    PmdgAircraft(VariableGateway* variableGateway, const AutomationStatus* status,
                 PmdgDataGateway* data, std::unique_ptr<PmdgTabletGateway> tablet,
                 PmdgAircraftSpec spec);

    [[nodiscard]] virtual const char* GetName() const = 0;

    [[nodiscard]] bool IsCargoVariant() const override;

    void OnTick() override;
    void OnLoadingStarted() override;
    void CloseAllDoors() override;
    void ClearOwnGroundEquipment() override;
    [[nodiscard]] DoorStatus GetDoorStatus() const override;
    [[nodiscard]] bool IsMainDeckCargoDoorStuck() const override;

    [[nodiscard]] bool RequiresEfbFlightPlan() const override { return true; }
    [[nodiscard]] bool IsFlightPlanLoaded() const override;
    [[nodiscard]] double GetPlannedFuelKg() const override;
    [[nodiscard]] double GetPlannedZfwKg() const override;
    [[nodiscard]] int GetPlannedPassengers() const override;
    [[nodiscard]] double GetEmptyZfwKg() const override;

    [[nodiscard]] double GetCurrentFuelKg() const override;
    void SetCurrentFuelKg(double fuelKg) override;
    [[nodiscard]] double GetCurrentZfwKg() const override;
    void SetCurrentZfwKg(double zfwKg) override;

    [[nodiscard]] bool SupportsStairsOrJetways() const override { return true; }
    [[nodiscard]] bool CompletesPushbackViaInterruptMenu() const override { return false; }
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return RefuelBy::Client; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return BoardBy::Client; }

    [[nodiscard]] bool ConsumeSmartSwitch() override;
    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] std::optional<GroundPowerStatus> GetGroundPowerStatus() const override;
    bool SetChocks(bool placed) override;
    [[nodiscard]] bool SupportsGroundPowerControl() const override { return true; }
    void SetGroundPower(bool on) override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

protected:
    [[nodiscard]] virtual bool HasVendorFlightPlan() const { return false; }

    VariableGateway* variableGateway_;
    const AutomationStatus* status_;
    PmdgDataGateway* data_;
    std::unique_ptr<PmdgTabletGateway> tablet_;

private:
    void SyncDoors();
    [[nodiscard]] std::optional<bool> DoorOpenAt(int slot) const;

    bool cargoVariant_;
    int doorSlots_;
    int mainDeckDoorSlot_;
    GsxDoorSync doors_;
    PmdgDoorReconciler doorReconciler_;
    PmdgGroundConnReconciler groundConn_;
    PmdgPayloadWriter payload_;
    PmdgRouteImport routeImport_;
    SmartSwitch smartSwitch_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGAIRCRAFT_H

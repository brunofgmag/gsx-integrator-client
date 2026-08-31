#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAX_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAX_H

#include <filesystem>
#include <optional>
#include <vector>

#include "../SmartSwitch.h"
#include "rules/IFly737MaxDoorsFollowLoaderCycleRule.h"
#include "rules/IFly737MaxWatchPlanFileRule.h"
#include "../../ifly/IFlyPlanFile.h"
#include "../../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

class IFly737Max final : public Aircraft
{
public:
    IFly737Max(VariableGateway* variableGateway, const AutomationStatus* status,
               std::optional<std::filesystem::path> planAppDataRoot = std::nullopt);

    [[nodiscard]] bool IsCargoVariant() const override;

    [[nodiscard]] const std::vector<AircraftRule*>& Rules() const override;
    void OnLoadingStarted() override {}

    [[nodiscard]] bool IsFlightPlanLoaded() const override;
    [[nodiscard]] double GetPlannedFuelKg() const override;
    [[nodiscard]] double GetPlannedZfwKg() const override;
    [[nodiscard]] int GetPlannedPassengers() const override;
    [[nodiscard]] double GetEmptyZfwKg() const override;

    [[nodiscard]] double GetCurrentFuelKg() const override;
    [[nodiscard]] double GetCurrentZfwKg() const override;
    void SetCurrentZfwKg(double zfwKg) override;

    [[nodiscard]] bool SupportsStairsOrJetways() const override { return true; }
    [[nodiscard]] bool RequiresEfbFlightPlan() const override { return true; }
    [[nodiscard]] bool CompletesPushbackViaInterruptMenu() const override { return false; }
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return RefuelBy::Gsx; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return BoardBy::Client; }

    [[nodiscard]] bool ConsumeSmartSwitch() override;
    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] DoorStatus GetDoorStatus() const override;
    void CloseAllDoors() override;
    void HoldDoorsClosed(bool hold) override;
    [[nodiscard]] bool IsHeldForDeparture() const;
    [[nodiscard]] bool WasCloseRequested() const;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsHeldInPlace() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    [[nodiscard]] bool IsBeaconOn() const;

    VariableGateway* variableGateway_;
    const AutomationStatus* status_;

    SmartSwitch smartSwitch_;
    IFlyPlanImport planImport_;
    IFly737MaxDoorsFollowLoaderCycleRule doorRule_;
    IFly737MaxWatchPlanFileRule planImportRule_;
    std::vector<AircraftRule*> rules_;
    double lastZfwKg_ = -1.0;
    bool heldForDeparture_ = false;
    bool closeRequested_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAX_H

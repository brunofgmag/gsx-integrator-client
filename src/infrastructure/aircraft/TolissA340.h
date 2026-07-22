#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TOLISSA340_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TOLISSA340_H

#include "SmartSwitch.h"
#include "../gsx/GsxDoorSync.h"
#include "../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

class TolissA340 final : public Aircraft
{
public:
    static constexpr auto kName = "ToLiss A340-600";

    TolissA340(VariableGateway* variableGateway, AutomationStatus* status, bool cargoVariant);

    [[nodiscard]] const char* GetName() const override;
    [[nodiscard]] bool IsCargoVariant() const override;

    void OnTick() override;
    void OnLoadingStarted() override;
    void CloseAllDoors() override;

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
    [[nodiscard]] bool CompletesPushbackViaInterruptMenu() const override { return true; }
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return RefuelBy::Self; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return BoardBy::Self; }

    [[nodiscard]] bool ConsumeSmartSwitch() override;
    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    [[nodiscard]] bool IsBeaconOn() const;
    [[nodiscard]] bool IsExternalPowerOn() const;
    void AdvanceUplink();
    void UpdateDoors();

    VariableGateway* variableGateway_;
    AutomationStatus* status_;
    bool cargoVariant_;
    GsxDoorSync doors_;
    SmartSwitch smartSwitch_;
    bool uplinkArmed_ = false;
    int uplinkStep_ = -1;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TOLISSA340_H

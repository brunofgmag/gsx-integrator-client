#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11_H

#include "../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

class TfdiMd11 final : public Aircraft
{
public:
    TfdiMd11(VariableGateway* variableGateway, AutomationStatus* status, bool cargo);

    [[nodiscard]] const char* GetName() const override;
    [[nodiscard]] bool IsCargoVariant() const override;

    void OnSlowTick() override;

    [[nodiscard]] bool IsFlightPlanLoaded() const override;
    [[nodiscard]] double GetPlannedFuelKg() const override;
    [[nodiscard]] double GetPlannedZfwKg() const override;
    [[nodiscard]] int GetPlannedPassengers() const override;
    [[nodiscard]] double GetEmptyZfwKg() const override;

    [[nodiscard]] double GetCurrentFuelKg() const override;
    void SetCurrentFuelKg(double fuelKg) override;
    [[nodiscard]] double GetCurrentZfwKg() const override;
    void SetCurrentZfwKg(double zfwKg) override;

    [[nodiscard]] bool SupportsProgressiveFuel() const override { return false; }
    [[nodiscard]] bool SupportsProgressiveLoad() const override { return false; }
    [[nodiscard]] bool SupportsStairsOrJetways() const override { return true; }

    [[nodiscard]] bool ConsumeSmartSwitch() override;
    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    void CommitEfbTargets() const;
    void SeedTargetsIfNeeded();

    [[nodiscard]] bool IsBeaconOn() const;

    VariableGateway* variableGateway_;
    AutomationStatus* status_;

    bool cargo_;
    bool smartSwitchResetPending_ = false;
    bool pendingEfbCommit_ = false;
    bool fuelTargetSeeded_ = false;
    bool zfwTargetSeeded_ = false;
    double targetFuelKg_ = 0.0;
    double lastFuelKg_ = 0.0;
    double targetZfwKg_ = 0.0;
    double lastZfwKg_ = 0.0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11_H

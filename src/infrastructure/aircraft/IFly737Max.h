#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAX_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAX_H

#include "../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

class IFly737Max final : public Aircraft
{
public:
    IFly737Max(VariableGateway* variableGateway, AutomationStatus* status);

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

    [[nodiscard]] bool SupportsProgressiveFuel() const override { return true; }
    [[nodiscard]] bool SupportsProgressiveLoad() const override { return true; }
    [[nodiscard]] bool SupportsStairsOrJetways() const override { return true; }
    [[nodiscard]] bool IsRefueledExternally() const override { return true; }

    [[nodiscard]] bool ConsumeSmartSwitch() override;
    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    [[nodiscard]] bool IsBeaconOn() const;

    VariableGateway* variableGateway_;
    AutomationStatus* status_;

    bool smartSwitchPressPending_ = false;
    double lastZfwKg_ = -1.0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAX_H

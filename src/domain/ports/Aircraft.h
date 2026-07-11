#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_AIRCRAFT_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_AIRCRAFT_H

class Aircraft
{
public:
    virtual ~Aircraft() = default;

    [[nodiscard]] virtual const char* GetName() const = 0;
    [[nodiscard]] virtual bool IsCargoVariant() const = 0;

    virtual void OnSlowTick() = 0;

    [[nodiscard]] virtual bool IsFlightPlanLoaded() const = 0;
    [[nodiscard]] virtual double GetPlannedFuelKg() const = 0;
    [[nodiscard]] virtual double GetPlannedZfwKg() const = 0;
    [[nodiscard]] virtual int GetPlannedPassengers() const = 0;
    [[nodiscard]] virtual double GetEmptyZfwKg() const = 0;

    [[nodiscard]] virtual double GetCurrentFuelKg() const = 0;
    virtual void SetCurrentFuelKg(double fuelKg) = 0;
    [[nodiscard]] virtual double GetCurrentZfwKg() const = 0;
    virtual void SetCurrentZfwKg(double zfwKg) = 0;

    [[nodiscard]] virtual bool SupportsProgressiveFuel() const = 0;
    [[nodiscard]] virtual bool SupportsProgressiveLoad() const = 0;
    [[nodiscard]] virtual bool SupportsStairsOrJetways() const = 0;
    [[nodiscard]] virtual bool IsRefueledExternally() const = 0;

    [[nodiscard]] virtual bool ConsumeSmartSwitch() = 0;

    [[nodiscard]] virtual bool IsPowered() const = 0;
    [[nodiscard]] virtual bool IsReadyToPush() const = 0 ;
    [[nodiscard]] virtual bool IsReadyToDeboard() const = 0;
    [[nodiscard]] virtual bool IsEngineRunning() const = 0;
    [[nodiscard]] virtual bool IsParkingBrakeSet() const = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_AIRCRAFT_H

#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_AIRCRAFT_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_AIRCRAFT_H

#include <optional>

#include "../model/GroundPowerStatus.h"
#include "../support/Weight.h"

enum class RefuelBy { Gsx, Self, Client };
enum class BoardBy { Self, Client };

class Aircraft
{
public:
    virtual ~Aircraft() = default;

    [[nodiscard]] virtual const char* GetName() const = 0;
    [[nodiscard]] virtual bool IsCargoVariant() const = 0;

    virtual void OnTick() {}
    virtual void OnSlowTick() {}
    virtual void OnLoadingStarted() = 0;

    [[nodiscard]] virtual bool RequiresEfbFlightPlan() const { return false; }
    [[nodiscard]] virtual bool IsFlightPlanLoaded() const = 0;
    [[nodiscard]] virtual double GetPlannedFuelKg() const = 0;
    [[nodiscard]] virtual double GetPlannedZfwKg() const = 0;
    [[nodiscard]] virtual int GetPlannedPassengers() const = 0;
    [[nodiscard]] virtual double GetEmptyZfwKg() const = 0;
    [[nodiscard]] virtual std::optional<WeightUnit> GetNativeWeightUnit() const { return std::nullopt; }

    [[nodiscard]] virtual double GetCurrentFuelKg() const = 0;
    virtual void SetCurrentFuelKg(double fuelKg) = 0;
    [[nodiscard]] virtual double GetCurrentZfwKg() const = 0;
    virtual void SetCurrentZfwKg(double zfwKg) = 0;

    [[nodiscard]] virtual bool SupportsStairsOrJetways() const = 0;
    [[nodiscard]] virtual bool CompletesPushbackViaInterruptMenu() const = 0;
    [[nodiscard]] virtual RefuelBy GetRefuelMethod() const = 0;
    [[nodiscard]] virtual BoardBy GetBoardMethod() const = 0;

    [[nodiscard]] virtual bool ConsumeSmartSwitch() = 0;

    [[nodiscard]] virtual bool IsPowered() const = 0;
    [[nodiscard]] virtual std::optional<GroundPowerStatus> GetGroundPowerStatus() const { return std::nullopt; }
    [[nodiscard]] virtual bool SupportsChocksControl() const { return false; }
    virtual void SetChocks(bool) {}
    [[nodiscard]] virtual bool SupportsGroundPowerControl() const { return false; }
    virtual void SetGroundPower(bool) {}
    virtual void CloseAllDoors() {}
    [[nodiscard]] virtual bool IsReadyToPush() const = 0 ;
    [[nodiscard]] virtual bool IsReadyToDeboard() const = 0;
    [[nodiscard]] virtual bool IsEngineRunning() const = 0;
    [[nodiscard]] virtual bool IsParkingBrakeSet() const = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_AIRCRAFT_H

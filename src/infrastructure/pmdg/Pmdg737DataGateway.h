#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737DATAGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737DATAGATEWAY_H

enum class Pmdg737Door
{
    FwdEntry,
    FwdService,
    Airstair,
    FwdCargo,
    AftCargo,
    MainCargo,
    EquipmentHatch,
    AftEntry,
    AftService,
    Count
};

class Pmdg737DataGateway
{
public:
    virtual ~Pmdg737DataGateway() = default;

    virtual void Poll() = 0;
    [[nodiscard]] virtual bool HasData() const = 0;

    [[nodiscard]] virtual int AircraftModel() const = 0;
    [[nodiscard]] virtual bool GroundPowerAvailable() const = 0;
    [[nodiscard]] virtual bool AnyMainBusPowered() const = 0;
    [[nodiscard]] virtual bool GroundConnAvailable() const = 0;
    [[nodiscard]] virtual bool BeaconOn() const = 0;
    [[nodiscard]] virtual bool ParkingBrakeOn() const = 0;
    [[nodiscard]] virtual bool IrsAligned() const = 0;
    [[nodiscard]] virtual double TotalFuelLbs() const = 0;

    virtual void ToggleDoor(Pmdg737Door door) = 0;
    virtual void SetInFlight(bool inFlight) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737DATAGATEWAY_H

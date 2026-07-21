#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777DATAGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777DATAGATEWAY_H

class Pmdg777DataGateway
{
public:
    virtual ~Pmdg777DataGateway() = default;

    virtual void Poll() = 0;
    [[nodiscard]] virtual bool HasData() const = 0;

    [[nodiscard]] virtual int AircraftModel() const = 0;
    [[nodiscard]] virtual bool ExtPowerConnected() const = 0;
    [[nodiscard]] virtual bool ExtPowerAvailable() const = 0;
    [[nodiscard]] virtual bool BeaconOn() const = 0;
    [[nodiscard]] virtual bool ParkingBrakeOn() const = 0;
    [[nodiscard]] virtual bool ApuRunning() const = 0;
    [[nodiscard]] virtual bool WheelChocksSet() const = 0;
    [[nodiscard]] virtual bool GroundConnAvailable() const = 0;
    [[nodiscard]] virtual bool IrsAligned() const = 0;
    [[nodiscard]] virtual bool HasFmcFlightPlan() const = 0;
    [[nodiscard]] virtual double TotalFuelLbs() const = 0;
    [[nodiscard]] virtual int DoorState(int index) const = 0;

    virtual void ToggleDoor(int index) = 0;
    virtual void KickDataRefresh() = 0;
    virtual void SetInFlight(bool inFlight) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777DATAGATEWAY_H

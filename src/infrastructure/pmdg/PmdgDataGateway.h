#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDATAGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDATAGATEWAY_H

class PmdgDataGateway
{
public:
    virtual ~PmdgDataGateway() = default;

    virtual void Poll() = 0;
    [[nodiscard]] virtual bool HasData() const = 0;

    [[nodiscard]] virtual bool BeaconOn() const = 0;
    [[nodiscard]] virtual bool ParkingBrakeOn() const = 0;

    virtual void SetInFlight(bool inFlight) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDATAGATEWAY_H

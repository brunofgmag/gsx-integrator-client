#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGGROUNDSOURCE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGGROUNDSOURCE_H

class PmdgGroundSource
{
public:
    virtual ~PmdgGroundSource() = default;

    [[nodiscard]] virtual bool HasAircraftPower() const = 0;
    [[nodiscard]] virtual bool GroundPowerConnected() const = 0;
    [[nodiscard]] virtual bool GroundPowerPresent() const { return GroundPowerConnected(); }
    [[nodiscard]] virtual bool ChocksSet() const = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGGROUNDSOURCE_H

#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777DATAGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777DATAGATEWAY_H

#include "PmdgDataGateway.h"

class Pmdg777DataGateway : public PmdgDataGateway
{
public:
    [[nodiscard]] virtual bool ExtPowerConnected() const = 0;
    [[nodiscard]] virtual bool ExtPowerAvailable() const = 0;
    [[nodiscard]] virtual bool ApuRunning() const = 0;
    [[nodiscard]] virtual bool WheelChocksSet() const = 0;
    [[nodiscard]] virtual bool HasFmcFlightPlan() const = 0;
    [[nodiscard]] virtual int DoorState(int index) const = 0;

    virtual void ToggleDoor(int index) = 0;
    virtual void KickDataRefresh() = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777DATAGATEWAY_H

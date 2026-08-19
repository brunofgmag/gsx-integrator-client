#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737DATAGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737DATAGATEWAY_H

#include "PmdgDataGateway.h"

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

class Pmdg737DataGateway : public PmdgDataGateway
{
public:
    [[nodiscard]] virtual bool GroundPowerAvailable() const = 0;
    [[nodiscard]] virtual bool AnyMainBusPowered() const = 0;

    virtual void ToggleDoor(Pmdg737Door door) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737DATAGATEWAY_H

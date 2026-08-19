#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDOORSOURCE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDOORSOURCE_H

#include "../gsx/GsxDoorSync.h"

enum class DoorObservation
{
    Unavailable,
    Moving,
    Open,
    Closed,
    Unknown
};

class PmdgDoorSource
{
public:
    virtual ~PmdgDoorSource() = default;

    [[nodiscard]] virtual int DoorSlotFor(GsxDoor door) const = 0;
    [[nodiscard]] virtual DoorObservation ObserveDoor(int slot) const = 0;
    virtual void ToggleDoor(int slot) = 0;
    virtual void RefreshDoors() = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGDOORSOURCE_H

#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDCHECKPOINT_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDCHECKPOINT_H

#include "TurnaroundData.h"
#include "TurnaroundPhase.h"
#include "../model/MemoryBag.h"

struct TurnaroundCheckpoint
{
    TurnaroundPhase phase = TurnaroundPhase::WaitingSupportedAircraft;
    TurnaroundData data;
    MemoryBag aircraftMemory;

    bool operator==(const TurnaroundCheckpoint&) const = default;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDCHECKPOINT_H

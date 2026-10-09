#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDRESTORE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDRESTORE_H

#include "TurnaroundData.h"
#include "TurnaroundFacts.h"
#include "TurnaroundPhase.h"

class Aircraft;

namespace turnaround
{
    [[nodiscard]] TurnaroundData SavedFields(const TurnaroundData& data);

    [[nodiscard]] TurnaroundData RestoreTurnaroundData(const TurnaroundData& saved,
                                                       TurnaroundPhase phase,
                                                       const Aircraft& aircraft,
                                                       bool gsxRestartedSinceSave);

    [[nodiscard]] TurnaroundFacts BuildTurnaroundFacts(TurnaroundPhase phase,
                                                       const TurnaroundData& data,
                                                       bool gsxRestartedSinceSave);
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDRESTORE_H

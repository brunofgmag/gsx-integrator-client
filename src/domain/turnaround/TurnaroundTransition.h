#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDTRANSITION_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDTRANSITION_H

#include "TurnaroundPhase.h"

enum class TransitionOrigin
{
    Reading,
    Pilot
};

struct TurnaroundTransition
{
    TurnaroundPhase next;
    int delayTicks = 0;
    TransitionOrigin origin = TransitionOrigin::Reading;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDTRANSITION_H

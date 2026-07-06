#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDTRANSITION_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDTRANSITION_H

#include "TurnaroundPhase.h"

struct TurnaroundTransition
{
    TurnaroundPhase next;
    int delayTicks = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDTRANSITION_H

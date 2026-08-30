#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_RULECONTEXT_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_RULECONTEXT_H

#include "PhaseNeeds.h"
#include "../TurnaroundPhase.h"

struct RuleContext
{
    TurnaroundPhase phase = TurnaroundPhase::WaitingSupportedAircraft;
    PhaseNeeds needs;
    int phaseTickCount = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_RULECONTEXT_H

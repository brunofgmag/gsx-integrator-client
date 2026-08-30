#ifndef GSX_INTEGRATOR_CLIENT_TESTS_AIRCRAFTTICKS_H
#define GSX_INTEGRATOR_CLIENT_TESTS_AIRCRAFTTICKS_H

#include <string>

#include "../src/domain/ports/Aircraft.h"
#include "../src/domain/ports/AircraftRule.h"
#include "../src/infrastructure/simvars/VariableGateway.h"

inline AircraftRule* FindRule(const Aircraft& aircraft, const std::string& name)
{
    for (AircraftRule* const rule : aircraft.Rules())
    {
        if (rule != nullptr && name == rule->Name())
        {
            return rule;
        }
    }

    return nullptr;
}

inline void RunAircraftRules(Aircraft& aircraft, VariableWriter& writer,
                             const RuleCadence cadence, const RuleContext& context)
{
    for (AircraftRule* const rule : aircraft.Rules())
    {
        if (rule == nullptr || rule->Cadence() != cadence)
        {
            continue;
        }

        rule->Evaluate(context);
        rule->Act(context, writer);
    }
}

inline void TickAircraft(Aircraft& aircraft, VariableWriter& writer, const RuleContext& context = {})
{
    aircraft.Observe();
    RunAircraftRules(aircraft, writer, RuleCadence::Fast, context);
}

inline void SlowTickAircraft(Aircraft& aircraft, VariableWriter& writer, const RuleContext& context = {})
{
    RunAircraftRules(aircraft, writer, RuleCadence::Slow, context);
}

#endif // GSX_INTEGRATOR_CLIENT_TESTS_AIRCRAFTTICKS_H

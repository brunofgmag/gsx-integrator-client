#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_RULEVERDICT_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_RULEVERDICT_H

struct RuleVerdict
{
    bool holds = false;
    int holdTicksAllowed = 0;
    const char* reason = "";

    [[nodiscard]] static RuleVerdict Pass()
    {
        return {};
    }

    [[nodiscard]] static RuleVerdict Hold(const int ticksAllowed, const char* reason)
    {
        return {true, ticksAllowed, reason};
    }
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_RULEVERDICT_H

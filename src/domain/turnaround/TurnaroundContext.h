#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDCONTEXT_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDCONTEXT_H

#include "TurnaroundData.h"

struct AutomationStatus;
struct AutomationSettings;
class Aircraft;
class GsxGateway;
class GsxMenuGateway;
class DomainLogger;

struct TurnaroundContext
{
    AutomationStatus* status = nullptr;
    const AutomationSettings* settings = nullptr;
    GsxMenuGateway* menuGateway = nullptr;
    GsxGateway* gsxGateway = nullptr;
    Aircraft* aircraft = nullptr;
    TurnaroundData data;
    DomainLogger* logger = nullptr;
    bool smartSwitchPressed = false;

    bool ConsumeSmartSwitch()
    {
        const bool pressed = smartSwitchPressed;
        smartSwitchPressed = false;
        return pressed;
    }

    bool TickCondition(const int tickValue) const
    {
        return data.stateTickCount != 0 && data.stateTickCount % tickValue == 0;
    }
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDCONTEXT_H

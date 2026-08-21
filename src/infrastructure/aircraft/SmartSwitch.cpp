#include "SmartSwitch.h"

#include <utility>

#include "../simvars/VariableGateway.h"
#include "../probe/ProbeLog.h"

SmartSwitch::SmartSwitch(VariableGateway& gateway, std::vector<std::string> lvars, Predicate pressed,
                         const std::optional<double> resetTo)
    : gateway_(gateway),
      lvars_(std::move(lvars)),
      pressed_(std::move(pressed)),
      resetTo_(resetTo)
{
}

void SmartSwitch::Subscribe()
{
    if (subscribed_)
    {
        return;
    }

    for (const auto& lvar : lvars_)
    {
        gateway_.SetFastRefresh(lvar);
    }

    subscribed_ = true;
}

bool SmartSwitch::Consume()
{
    if (!subscribed_)
    {
        return false;
    }

    bool active = false;
    for (const auto& lvar : lvars_)
    {
        const LVarSpan span = gateway_.ConsumeLVarSpan(lvar);
        probe::Change("swtch." + lvar,
                      QStringLiteral("swtch %1 recv=%2 span=[%3..%4] pressed=%5 pending=%6")
                      .arg(QString::fromStdString(lvar))
                      .arg(span.received)
                      .arg(span.min, 0, 'f', 3)
                      .arg(span.max, 0, 'f', 3)
                      .arg(span.received && pressed_(span.min, span.max))
                      .arg(pending_));

        if (span.received && pressed_(span.min, span.max))
        {
            active = true;
        }
    }

    if (!active)
    {
        pending_ = false;
        return false;
    }

    if (resetTo_)
    {
        for (const auto& lvar : lvars_)
        {
            gateway_.SetLVar(lvar, *resetTo_);
        }
    }

    if (pending_)
    {
        return false;
    }

    pending_ = true;
    return true;
}

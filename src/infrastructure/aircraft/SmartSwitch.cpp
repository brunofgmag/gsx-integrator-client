#include "SmartSwitch.h"

#include <utility>

#include <QDateTime>

#include "../simvars/VariableGateway.h"
#include "../probe/ProbeLog.h"

SmartSwitch::SmartSwitch(VariableGateway& gateway, std::vector<std::string> lvars, Predicate pressed,
                         const std::optional<double> resetTo)
    : gateway_(gateway),
      lvars_(std::move(lvars)),
      pressed_(std::move(pressed)),
      resetTo_(resetTo)
{
    nowMs_ = [] { return static_cast<long long>(QDateTime::currentMSecsSinceEpoch()); };
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

    lastConsumeMs_ = nowMs_();
    subscribed_ = true;
}

bool SmartSwitch::Consume()
{
    if (!subscribed_)
    {
        return false;
    }

    const long long now = nowMs_();
    const bool stale = now - lastConsumeMs_ > kMaxGapMs;
    lastConsumeMs_ = now;

    bool active = false;
    for (const auto& lvar : lvars_)
    {
        const LVarSpan span = gateway_.ConsumeLVarSpan(lvar);
        const bool pressed = span.received && pressed_(span.min, span.max);

        if (probe::IsOn())
        {
            probe::Change("swtch." + lvar,
                          QStringLiteral("swtch %1 recv=%2 span=[%3..%4] pressed=%5 pending=%6 stale=%7")
                          .arg(QString::fromStdString(lvar))
                          .arg(span.received)
                          .arg(span.min, 0, 'f', 3)
                          .arg(span.max, 0, 'f', 3)
                          .arg(pressed)
                          .arg(pending_)
                          .arg(stale));
        }

        if (pressed)
        {
            active = true;
        }
    }

    if (stale)
    {
        return false;
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

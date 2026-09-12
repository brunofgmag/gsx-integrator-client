#include "Fss727OwnGpuFollowsTheGsxUnitRule.h"

#include <QtCore/QString>

#include "../../../logging/LogMacros.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"
#include "../../../../domain/ports/GsxGateway.h"

namespace
{
    constexpr auto kRuleName = "fss-727-own-gpu-follows-the-gsx-unit";

    constexpr auto kGpuAvailableLVar = "FSS_B727_GPU_AVAIL";
    constexpr double kGpuRaised = 1.0;
    constexpr double kGpuStowed = 0.0;
}

Fss727OwnGpuFollowsTheGsxUnitRule::Fss727OwnGpuFollowsTheGsxUnitRule(const GsxGateway* gsxGateway)
    : gsxGateway_(gsxGateway)
{
}

const char* Fss727OwnGpuFollowsTheGsxUnitRule::Name() const
{
    return kRuleName;
}

RuleVerdict Fss727OwnGpuFollowsTheGsxUnitRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void Fss727OwnGpuFollowsTheGsxUnitRule::Act(const RuleContext&, VariableWriter& writer)
{
    if (gsxGateway_ == nullptr)
    {
        return;
    }

    const GroundPowerStatus unit = gsxGateway_->GetGpuStatus();

    if (unit == GroundPowerStatus::Connected)
    {
        if (!raised_)
        {
            probe::Line(QStringLiteral("write gpu FSS_B727_GPU_AVAIL=1"));
            writer.SetLVar(kGpuAvailableLVar, kGpuRaised);
            raised_ = true;

            LOG_INFO("FSS 727 own ground power raised: the GSX unit is connected, the EXT POWER switch is the pilot's");
        }

        return;
    }

    if (unit == GroundPowerStatus::Unknown || !raised_)
    {
        return;
    }

    probe::Line(QStringLiteral("write gpu FSS_B727_GPU_AVAIL=0"));
    writer.SetLVar(kGpuAvailableLVar, kGpuStowed);
    raised_ = false;

    LOG_INFO("FSS 727 own ground power stowed: the GSX unit left");
}

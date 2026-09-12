#include "Fss727MainDeckClosesByTheCargoPanelRule.h"

#include <optional>
#include <QtCore/QString>

#include "../Fss727.h"
#include "../../../logging/LogMacros.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"
#include "../../../../domain/ports/GsxGateway.h"

namespace
{
    constexpr auto kRuleName = "fss-727-main-deck-closes-by-the-cargo-panel";

    constexpr auto kMasterCoverLVar = "FSS_B727_CDP_MASTER_POWER_COVER_SWITCH";
    constexpr auto kMasterPowerLVar = "FSS_B727_CDP_MASTER_POWER_SWITCH";
    constexpr auto kCargoDoorSwitchLVar = "FSS_B727_CDP_CARGO_DOOR_SWITCH";

    constexpr double kSwitchOn = 1.0;
    constexpr double kSwitchOff = 0.0;
}

Fss727MainDeckClosesByTheCargoPanelRule::Fss727MainDeckClosesByTheCargoPanelRule(
    const Fss727& aircraft, const GsxGateway* gsxGateway)
    : aircraft_(&aircraft), gsxGateway_(gsxGateway)
{
}

const char* Fss727MainDeckClosesByTheCargoPanelRule::Name() const
{
    return kRuleName;
}

RuleVerdict Fss727MainDeckClosesByTheCargoPanelRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void Fss727MainDeckClosesByTheCargoPanelRule::Act(const RuleContext&, VariableWriter& writer)
{
    const int requests = aircraft_->MainDeckCloseRequests();
    if (requests == servedRequests_ && !masterOn_)
    {
        return;
    }

    if (IsGsxWorkingTheCargoDoors())
    {
        return;
    }

    const std::optional<bool> closed = aircraft_->IsMainDeckClosed();
    if (!closed.has_value())
    {
        return;
    }

    if (*closed)
    {
        if (masterOn_)
        {
            probe::Line(QStringLiteral("write panel master=0"));
            writer.SetLVar(kMasterPowerLVar, kSwitchOff);
            masterOn_ = false;

            LOG_INFO("FSS 727 main deck door closed: the cargo panel master goes off");
        }

        servedRequests_ = requests;

        return;
    }

    if (masterOn_)
    {
        return;
    }

    probe::Line(QStringLiteral("write panel cover=1 master=1 door=0"));
    writer.SetLVar(kMasterCoverLVar, kSwitchOn);
    writer.SetLVar(kMasterPowerLVar, kSwitchOn);
    writer.SetLVar(kCargoDoorSwitchLVar, kSwitchOff);
    masterOn_ = true;

    LOG_INFO("FSS 727 main deck door commanded closed, and the panel master stays on for the whole travel");
}

bool Fss727MainDeckClosesByTheCargoPanelRule::IsGsxWorkingTheCargoDoors() const
{
    if (gsxGateway_ == nullptr)
    {
        return false;
    }

    const auto isUnderway = [](const GsxStateStatus state)
    {
        return state == GsxStateStatus::Requested || state == GsxStateStatus::Active;
    };

    return isUnderway(gsxGateway_->GetStateStatus(GsxState::Boarding))
        || isUnderway(gsxGateway_->GetStateStatus(GsxState::Deboarding));
}

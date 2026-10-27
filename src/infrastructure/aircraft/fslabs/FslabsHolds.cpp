#include "FslabsHolds.h"

#include <optional>
#include "../../gsx/GsxLVars.h"
#include "../../logging/LogMacros.h"
#include "../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kForwardName = "forward";
    constexpr auto kAftName = "aft";
    constexpr double kNoLoaderState = 0.0;
    constexpr int kTicksAToggleGetsToShowUp = 20;
    constexpr int kTogglesBeforeGivingUp = 3;
}

FslabsHolds::FslabsHolds(VariableGateway& variables, FslabsGroundGateway& ground)
    : variables_(&variables),
      ground_(&ground),
      follows_{{
          {FslabsHold::Forward, gsx::lvars::kBaggageLoaderFrontState, kForwardName},
          {FslabsHold::Aft, gsx::lvars::kBaggageLoaderRearState, kAftName}
      }}
{
}

void FslabsHolds::Advance()
{
    for (Follow& follow : follows_)
    {
        Advance(follow);
    }
}

void FslabsHolds::ResetForNewTurnaround()
{
    for (Follow& follow : follows_)
    {
        follow = Follow{follow.hold, follow.loaderStateLVar, follow.name};
    }
}

void FslabsHolds::Advance(Follow& follow)
{
    ++follow.ticksSinceToggle;

    if (!variables_->HasReceivedLVar(follow.loaderStateLVar))
    {
        return;
    }

    const double loaderState = variables_->GetLVar(follow.loaderStateLVar, kNoLoaderState);
    const bool stillAtTheDoor =
        gsx::states::IsLoaderAtDoor(loaderState) || loaderState == gsx::states::kLoaderRetracting;
    if (stillAtTheDoor)
    {
        follow.closeIsDue = false;
        follow.toggles = 0;
        follow.loaderWasServing = follow.loaderWasServing || gsx::states::IsLoaderServingTheDoor(loaderState);
    }
    else if (follow.loaderWasServing)
    {
        follow.closeIsDue = true;
        follow.loaderWasServing = false;
    }

    if (follow.closeIsDue)
    {
        CloseTheHold(follow);
    }
}

void FslabsHolds::CloseTheHold(Follow& follow)
{
    const std::optional<FslabsHoldState> state = ground_->HoldState(follow.hold);
    if (!ground_->IsAvailable() || !state.has_value())
    {
        return;
    }

    if (*state == FslabsHoldState::Closed)
    {
        follow.closeIsDue = false;

        return;
    }

    const bool waitsForTheLastToggle = follow.toggles > 0 && follow.ticksSinceToggle < kTicksAToggleGetsToShowUp;
    if (*state != FslabsHoldState::Open || waitsForTheLastToggle)
    {
        return;
    }

    if (follow.toggles >= kTogglesBeforeGivingUp)
    {
        follow.closeIsDue = false;

        LOG_WARN("FSLabs %s cargo hold is still open after %d toggles: leaving it to the aircraft",
                 follow.name, follow.toggles);

        return;
    }

    ground_->ToggleHold(follow.hold);
    ++follow.toggles;
    follow.ticksSinceToggle = 0;

    LOG_INFO("FSLabs %s cargo hold: the loader left, closing it (toggle %d)", follow.name, follow.toggles);
}

#include "TfdiMd11CargoDoorsFollowLoaderRule.h"

#include <QtCore/QString>

#include "../../DoorReading.h"
#include "../../../gsx/GsxDoorSync.h"
#include "../../../gsx/GsxLVars.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "tfdi-md11-cargo-doors-follow-loader";

    constexpr auto kCargoDoor1RLVar = "MD11_EXT_DOOR_CMD_CARGO_1R";
    constexpr auto kCargoDoor2RLVar = "MD11_EXT_DOOR_CMD_CARGO_2R";
    constexpr auto kCargoDoorMainLVar = "MD11_EXT_DOOR_CMD_CARGO_MAIN";

    constexpr auto kCargoDoor1RReadingLVar = "MD11_EXT_DOOR_CARGO_1R";
    constexpr auto kCargoDoor2RReadingLVar = "MD11_EXT_DOOR_CARGO_2R";
    constexpr auto kCargoDoorMainReadingLVar = "MD11_EXT_DOOR_CARGO_MAIN";

    constexpr double kDoorOpen = 100.0;
    constexpr double kDoorClosed = 0.0;
}

TfdiMd11CargoDoorsFollowLoaderRule::TfdiMd11CargoDoorsFollowLoaderRule(VariableReader& variables,
                                                                      GsxDoorSync& doors,
                                                                      const bool cargoVariant)
    : variables_(&variables), doors_(&doors), cargoVariant_(cargoVariant)
{
}

const char* TfdiMd11CargoDoorsFollowLoaderRule::Name() const
{
    return kRuleName;
}

RuleVerdict TfdiMd11CargoDoorsFollowLoaderRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void TfdiMd11CargoDoorsFollowLoaderRule::Act(const RuleContext&, VariableWriter& writer)
{
    if (variables_->GetLVar(gsx::lvars::kCouatlStarted, 0.0) < 1.0)
    {
        return;
    }

    FollowLoader(writer, {.loaderState = gsx::lvars::kBaggageLoaderFrontState, .command = kCargoDoor1RLVar, .reading = kCargoDoor1RReadingLVar}, targets_.fwd);
    FollowLoader(writer, {.loaderState = gsx::lvars::kBaggageLoaderRearState, .command = kCargoDoor2RLVar, .reading = kCargoDoor2RReadingLVar}, targets_.aft);

    if (cargoVariant_)
    {
        FollowLoader(writer, {.loaderState = gsx::lvars::kBaggageLoaderMainState, .command = kCargoDoorMainLVar, .reading = kCargoDoorMainReadingLVar}, targets_.main);
    }
}

TfdiMd11CargoDoorsFollowLoaderRule::DoorTargets TfdiMd11CargoDoorsFollowLoaderRule::Targets() const
{
    return targets_;
}

void TfdiMd11CargoDoorsFollowLoaderRule::RestoreTargets(const DoorTargets& targets)
{
    targets_ = targets;
}

void TfdiMd11CargoDoorsFollowLoaderRule::FollowLoader(VariableWriter& writer, const DoorLVars& door,
                                                      std::optional<double>& lastDoorTarget) const
{
    if (!variables_->HasReceivedLVar(door.loaderState))
    {
        return;
    }

    const double loaderState = doors_->VehicleState(door.loaderState, 0.0);
    const double doorTarget = gsx::states::IsLoaderServingTheDoor(loaderState) ? kDoorOpen : kDoorClosed;
    const bool wantsOpen = doorTarget == kDoorOpen;
    const bool isAlreadyTheTarget = lastDoorTarget == doorTarget;
    const bool hasNothingOfItsOwnToClose = !lastDoorTarget.has_value() && !wantsOpen;

    if (isAlreadyTheTarget || hasNothingOfItsOwnToClose)
    {
        return;
    }

    const std::optional<bool> isOpen = doors::OpenAboveZero(*variables_, door.reading);

    if (!isOpen.has_value())
    {
        return;
    }

    if (*isOpen != wantsOpen)
    {
        probe::Line(probe::Channel::Writes, QStringLiteral("write cargo %1=%2 %3=%4 %5=%6")
                        .arg(QLatin1String(door.command))
                        .arg(doorTarget)
                        .arg(QLatin1String(door.loaderState))
                        .arg(loaderState)
                        .arg(QLatin1String(door.reading))
                        .arg(*isOpen ? 1 : 0));
        writer.SetLVar(door.command, doorTarget);
    }

    lastDoorTarget = doorTarget;
}

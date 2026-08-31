#include "TfdiMd11CargoDoorsFollowLoaderRule.h"

#include "../../../gsx/GsxLVars.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "tfdi-md11-cargo-doors-follow-loader";

    constexpr auto kCargoDoor1RLVar = "MD11_EXT_DOOR_CMD_CARGO_1R";
    constexpr auto kCargoDoor2RLVar = "MD11_EXT_DOOR_CMD_CARGO_2R";
    constexpr auto kCargoDoorMainLVar = "MD11_EXT_DOOR_CMD_CARGO_MAIN";

    constexpr double kDoorOpen = 100.0;
    constexpr double kDoorClosed = 0.0;
}

TfdiMd11CargoDoorsFollowLoaderRule::TfdiMd11CargoDoorsFollowLoaderRule(VariableReader& variables,
                                                                      const bool cargoVariant)
    : variables_(&variables), cargoVariant_(cargoVariant)
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

    FollowLoader(writer, gsx::lvars::kBaggageLoaderFrontState, kCargoDoor1RLVar, fwdDoorTarget_);
    FollowLoader(writer, gsx::lvars::kBaggageLoaderRearState, kCargoDoor2RLVar, aftDoorTarget_);

    if (cargoVariant_)
    {
        FollowLoader(writer, gsx::lvars::kBaggageLoaderMainState, kCargoDoorMainLVar, mainDoorTarget_);
    }
}

void TfdiMd11CargoDoorsFollowLoaderRule::FollowLoader(VariableWriter& writer, const char* loaderStateLVar,
                                                      const char* doorCmdLVar, double& lastDoorTarget) const
{
    const double loaderState = variables_->GetLVar(loaderStateLVar, 0.0);
    const double doorTarget = gsx::states::IsLoaderArriving(loaderState) ? kDoorOpen : kDoorClosed;

    if (doorTarget != lastDoorTarget)
    {
        writer.SetLVar(doorCmdLVar, doorTarget);
        lastDoorTarget = doorTarget;
    }
}

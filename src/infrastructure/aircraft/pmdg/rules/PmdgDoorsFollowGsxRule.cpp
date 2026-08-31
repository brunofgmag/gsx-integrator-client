#include "PmdgDoorsFollowGsxRule.h"

#include "../../../gsx/GsxDoorSync.h"
#include "../../../gsx/GsxLVars.h"
#include "../../../pmdg/PmdgDataGateway.h"
#include "../../../pmdg/PmdgDoorReconciler.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "pmdg-doors-follow-gsx";

    constexpr double kGsxDoorAutomationOn = 1.0;
    constexpr double kGsxDoorAutomationOff = 0.0;
}

PmdgDoorsFollowGsxRule::PmdgDoorsFollowGsxRule(VariableReader& variables, const PmdgDataGateway& data,
                                               GsxDoorSync& doors, PmdgDoorReconciler& reconciler,
                                               const bool cargoVariant, const int mainDeckDoorSlot)
    : variables_(&variables), data_(&data), doors_(&doors), reconciler_(&reconciler),
      cargoVariant_(cargoVariant), mainDeckDoorSlot_(mainDeckDoorSlot)
{
}

const char* PmdgDoorsFollowGsxRule::Name() const
{
    return kRuleName;
}

RuleVerdict PmdgDoorsFollowGsxRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void PmdgDoorsFollowGsxRule::Act(const RuleContext&, VariableWriter& writer)
{
    if (!data_->HasData())
    {
        return;
    }

    if (variables_->GetLVar(gsx::lvars::kAutomationDoors, kGsxDoorAutomationOn) != kGsxDoorAutomationOff)
    {
        writer.SetLVar(gsx::lvars::kAutomationDoors, kGsxDoorAutomationOff);
    }

    doors_->Sync([this](const GsxDoor door, const bool open) { reconciler_->SetDesired(door, open); });

    if (cargoVariant_)
    {
        SyncMainDeckDoor();
    }

    reconciler_->Reconcile();
}

void PmdgDoorsFollowGsxRule::SyncMainDeckDoor()
{
    const bool loaderPresent = gsx::states::IsLoaderArriving(
        doors_->VehicleState(gsx::lvars::kBaggageLoaderMainState, 0.0));

    if (loaderPresent)
    {
        mainDeckDoorTaken_ = true;
    }

    if (!mainDeckDoorTaken_)
    {
        return;
    }

    reconciler_->SetSlotDesired(mainDeckDoorSlot_, loaderPresent);
}

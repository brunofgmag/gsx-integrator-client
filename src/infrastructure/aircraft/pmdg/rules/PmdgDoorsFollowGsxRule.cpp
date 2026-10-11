#include "PmdgDoorsFollowGsxRule.h"

#include "../../../gsx/GsxDoorSync.h"
#include "../../../gsx/GsxLVars.h"
#include "../../../pmdg/PmdgDataGateway.h"
#include "../../../pmdg/PmdgDoorReconciler.h"

namespace
{
    constexpr auto kRuleName = "pmdg-doors-follow-gsx";
    constexpr auto kMainDeckDoorTakenKey = "pmdgDoorRule.mainDeckDoorTaken";
}

PmdgDoorsFollowGsxRule::PmdgDoorsFollowGsxRule(const PmdgDataGateway& data, GsxDoorSync& doors,
                                               PmdgDoorReconciler& reconciler, const bool cargoVariant,
                                               const int mainDeckDoorSlot)
    : data_(&data), doors_(&doors), reconciler_(&reconciler),
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

void PmdgDoorsFollowGsxRule::Act(const RuleContext&, VariableWriter&)
{
    if (!data_->HasData())
    {
        return;
    }

    doors_->Sync([this](const GsxDoor door, const bool open) { reconciler_->SetDesired(door, open); });

    if (cargoVariant_)
    {
        SyncMainDeckDoor();
    }

    reconciler_->Reconcile();
}

void PmdgDoorsFollowGsxRule::ForgetMainDeckDoor()
{
    mainDeckDoorTaken_ = false;
}

void PmdgDoorsFollowGsxRule::AppendMemory(MemoryBag& memory) const
{
    if (mainDeckDoorTaken_)
    {
        memory.PutFlag(kMainDeckDoorTakenKey, true);
    }
}

void PmdgDoorsFollowGsxRule::RestoreMemory(const MemoryBag& memory)
{
    mainDeckDoorTaken_ = cargoVariant_ && memory.Flag(kMainDeckDoorTakenKey, false);
}

void PmdgDoorsFollowGsxRule::SyncMainDeckDoor()
{
    const bool loaderPresent = gsx::states::IsLoaderServingTheDoor(
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

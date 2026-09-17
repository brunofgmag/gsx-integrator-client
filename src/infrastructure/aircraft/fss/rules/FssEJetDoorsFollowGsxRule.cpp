#include "FssEJetDoorsFollowGsxRule.h"

#include <algorithm>
#include <cstddef>
#include <QtCore/QString>

#include "../FssEJet.h"
#include "../../../gsx/GsxLVars.h"
#include "../../../logging/LogMacros.h"
#include "../../../probe/ProbeLog.h"
#include "../../../simvars/VariableGateway.h"

namespace
{
    constexpr auto kRuleName = "fss-ejet-doors-follow-gsx";

    constexpr double kDoorOpen = 1.0;
    constexpr double kDoorClosed = 0.0;

    constexpr int kPassengerDoorReaffirmTicks = 14;
    constexpr int kCargoDoorReaffirmTicks = 2;
    constexpr int kMaxReaffirms = 2;
    constexpr int kAckPhaseDivisor = 10;

    constexpr auto kMainDeckReqLVar = "FSS_GNDSVC_CARGO_MAIN_REQ";
    constexpr auto kMainDeckOpenLVar = "FSS_EXX_DOOR_CARGO_MAIN_OPEN";
    constexpr int kMainDeckReaffirmTicks = kCargoDoorReaffirmTicks;
    constexpr int kMainDeckPowerHoldTicks = 120;
    constexpr auto kMainDeckPowerHoldReason = "the FSS E-Jet freighter is not energized yet";

    constexpr std::array<FssEJetDoorSlot, 6> kDoorSlots{{
        {GsxDoor::FwdPax, "FSS_GNDSVC_MAINDOOR_FWD_L_REQ", "FSS_FLTCREW_MAINDOOR_FWD_L_REQ",
         "FSS_EXX_DOOR_FWD_L_OPEN", kPassengerDoorReaffirmTicks},
        {GsxDoor::AftPax, "FSS_GNDSVC_MAINDOOR_AFT_L_REQ", "FSS_FLTCREW_MAINDOOR_AFT_L_REQ",
         "FSS_EXX_DOOR_AFT_L_OPEN", kPassengerDoorReaffirmTicks},
        {GsxDoor::FwdCatering, "FSS_GNDSVC_MAINDOOR_FWD_R_REQ", "FSS_FLTCREW_MAINDOOR_FWD_R_REQ",
         "FSS_EXX_DOOR_FWD_R_OPEN", kPassengerDoorReaffirmTicks},
        {GsxDoor::AftCatering, "FSS_GNDSVC_MAINDOOR_AFT_R_REQ", "FSS_FLTCREW_MAINDOOR_AFT_R_REQ",
         "FSS_EXX_DOOR_AFT_R_OPEN", kPassengerDoorReaffirmTicks},
        {GsxDoor::FwdCargo, "FSS_GNDSVC_CARGO_FWD_REQ", nullptr,
         "FSS_EXX_DOOR_CARGO_FWD_OPEN", kCargoDoorReaffirmTicks},
        {GsxDoor::AftCargo, "FSS_GNDSVC_CARGO_AFT_REQ", nullptr,
         "FSS_EXX_DOOR_CARGO_AFT_OPEN", kCargoDoorReaffirmTicks}
    }};

    bool IsDoorHiddenFromTheFreighter(const GsxDoor door)
    {
        return door == GsxDoor::AftPax || door == GsxDoor::AftCatering;
    }
}

FssEJetDoorsFollowGsxRule::FssEJetDoorsFollowGsxRule(VariableReader& variables, GsxDoorSync& doors,
                                                     const FssEJet& aircraft, const bool cargoVariant)
    : variables_(&variables), doors_(&doors), aircraft_(&aircraft), cargoVariant_(cargoVariant)
{
}

const char* FssEJetDoorsFollowGsxRule::Name() const
{
    return kRuleName;
}

void FssEJetDoorsFollowGsxRule::RequestCloseAll()
{
    ++closeAllRequests_;
}

RuleVerdict FssEJetDoorsFollowGsxRule::Evaluate(const RuleContext& context)
{
    if (cargoVariant_ && context.needs.loading
        && IsMainLoaderWaitingForTheDeck() && !IsAircraftEnergized())
    {
        return RuleVerdict::Hold(kMainDeckPowerHoldTicks, kMainDeckPowerHoldReason);
    }

    return RuleVerdict::Pass();
}

void FssEJetDoorsFollowGsxRule::Act(const RuleContext&, VariableWriter& writer)
{
    const bool closeAllPending = closeAllRequests_ != servedCloseAllRequests_;

    if (closeAllPending)
    {
        doors_->CloseAll([this](const GsxDoor door, const bool open) { SetDesired(door, open); });
        servedCloseAllRequests_ = closeAllRequests_;
    }
    else
    {
        doors_->Sync([this](const GsxDoor door, const bool open) { SetDesired(door, open); });
    }

    for (std::size_t index = 0; index < kDoorSlots.size(); ++index)
    {
        ReconcileSlot(index, writer);
    }

    if (cargoVariant_)
    {
        ReconcileMainDeck(closeAllPending, writer);
    }
}

void FssEJetDoorsFollowGsxRule::SetDesired(const GsxDoor door, const bool open)
{
    if (cargoVariant_ && IsDoorHiddenFromTheFreighter(door))
    {
        return;
    }

    const auto match = std::ranges::find(kDoorSlots, door, &FssEJetDoorSlot::door);
    if (match == kDoorSlots.end())
    {
        return;
    }

    const auto index = static_cast<std::size_t>(std::distance(kDoorSlots.begin(), match));
    states_[index].desired = open;
}

void FssEJetDoorsFollowGsxRule::ReconcileSlot(const std::size_t index, VariableWriter& writer)
{
    SlotState& state = states_[index];
    if (!state.desired.has_value())
    {
        return;
    }

    const FssEJetDoorSlot& slot = kDoorSlots[index];
    const bool wantOpen = *state.desired;

    if (state.commanded != state.desired)
    {
        state.commanded = state.desired;
        state.ticksSinceCommand = 0;
        state.attempts = 0;
        WriteRequest(slot, wantOpen, writer);

        return;
    }

    if (IsConfirmed(slot, wantOpen))
    {
        state.attempts = 0;

        return;
    }

    ++state.ticksSinceCommand;
    if (state.ticksSinceCommand >= slot.reaffirmTicks && state.attempts < kMaxReaffirms)
    {
        state.ticksSinceCommand = 0;
        ++state.attempts;
        WriteRequest(slot, wantOpen, writer);
    }
}

bool FssEJetDoorsFollowGsxRule::IsConfirmed(const FssEJetDoorSlot& slot, const bool wantOpen) const
{
    if (slot.ackLVar != nullptr)
    {
        if (!variables_->HasReceivedLVar(slot.ackLVar))
        {
            return false;
        }

        const auto ack = static_cast<int>(variables_->GetLVar(slot.ackLVar, 0.0));

        return (ack % kAckPhaseDivisor) == (wantOpen ? 1 : 0);
    }

    return IsOpenLVarConfirmed(slot.openLVar, wantOpen);
}

bool FssEJetDoorsFollowGsxRule::IsOpenLVarConfirmed(const char* openLVar, const bool wantOpen) const
{
    if (!variables_->HasReceivedLVar(openLVar))
    {
        return false;
    }

    return (variables_->GetLVar(openLVar, 0.0) > 0.0) == wantOpen;
}

void FssEJetDoorsFollowGsxRule::WriteRequest(const FssEJetDoorSlot& slot, const bool open, VariableWriter& writer)
{
    probe::Line(QStringLiteral("write door req=%1 open=%2").arg(QLatin1String(slot.reqLVar)).arg(open ? 1 : 0));
    writer.SetLVar(slot.reqLVar, open ? kDoorOpen : kDoorClosed);

    LOG_INFO("FSS E-Jet door commanded via %s: %s", slot.reqLVar, open ? "open" : "closed");
}

void FssEJetDoorsFollowGsxRule::ReconcileMainDeck(const bool forceClosed, VariableWriter& writer)
{
    const bool wantOpen = !forceClosed && IsAircraftEnergized() && IsMainLoaderWaitingForTheDeck();
    const bool pending = wantOpen ? mainDeckCommanded_ != true : mainDeckCommanded_ == true;

    if (pending)
    {
        mainDeckCommanded_ = wantOpen;
        mainDeckTicksSinceCommand_ = 0;
        mainDeckAttempts_ = 0;

        WriteMainDeckRequest(wantOpen, writer);

        LOG_INFO("FSS E-Jet freighter main deck door commanded %s", wantOpen ? "open" : "closed");

        return;
    }

    if (!mainDeckCommanded_.has_value())
    {
        return;
    }

    if (IsOpenLVarConfirmed(kMainDeckOpenLVar, wantOpen))
    {
        mainDeckAttempts_ = 0;

        return;
    }

    ++mainDeckTicksSinceCommand_;
    if (mainDeckTicksSinceCommand_ >= kMainDeckReaffirmTicks && mainDeckAttempts_ < kMaxReaffirms)
    {
        mainDeckTicksSinceCommand_ = 0;
        ++mainDeckAttempts_;

        WriteMainDeckRequest(wantOpen, writer);
    }
}

void FssEJetDoorsFollowGsxRule::WriteMainDeckRequest(const bool open, VariableWriter& writer)
{
    probe::Line(QStringLiteral("write door req=%1 open=%2").arg(QLatin1String(kMainDeckReqLVar)).arg(open ? 1 : 0));
    writer.SetLVar(kMainDeckReqLVar, open ? kDoorOpen : kDoorClosed);
}

bool FssEJetDoorsFollowGsxRule::IsMainLoaderWaitingForTheDeck() const
{
    return gsx::states::IsLoaderArriving(doors_->VehicleState(gsx::lvars::kBaggageLoaderMainState, 0.0));
}

bool FssEJetDoorsFollowGsxRule::IsAircraftEnergized() const
{
    return aircraft_->IsPowered();
}


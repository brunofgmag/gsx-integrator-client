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

    constexpr std::array<FssEJetDoorSlot, kFssEJetDoorSlotCount> kDoorSlots{{
        {.door = GsxDoor::FwdPax, .reqLVar = "FSS_GNDSVC_MAINDOOR_FWD_L_REQ", .ackLVar = "FSS_FLTCREW_MAINDOOR_FWD_L_REQ",
         .openLVar = "FSS_EXX_DOOR_FWD_L_OPEN", .reaffirmTicks = kPassengerDoorReaffirmTicks},
        {.door = GsxDoor::AftPax, .reqLVar = "FSS_GNDSVC_MAINDOOR_AFT_L_REQ", .ackLVar = "FSS_FLTCREW_MAINDOOR_AFT_L_REQ",
         .openLVar = "FSS_EXX_DOOR_AFT_L_OPEN", .reaffirmTicks = kPassengerDoorReaffirmTicks},
        {.door = GsxDoor::FwdCatering, .reqLVar = "FSS_GNDSVC_MAINDOOR_FWD_R_REQ", .ackLVar = "FSS_FLTCREW_MAINDOOR_FWD_R_REQ",
         .openLVar = "FSS_EXX_DOOR_FWD_R_OPEN", .reaffirmTicks = kPassengerDoorReaffirmTicks},
        {.door = GsxDoor::AftCatering, .reqLVar = "FSS_GNDSVC_MAINDOOR_AFT_R_REQ", .ackLVar = "FSS_FLTCREW_MAINDOOR_AFT_R_REQ",
         .openLVar = "FSS_EXX_DOOR_AFT_R_OPEN", .reaffirmTicks = kPassengerDoorReaffirmTicks},
        {.door = GsxDoor::FwdCargo, .reqLVar = "FSS_GNDSVC_CARGO_FWD_REQ", .ackLVar = nullptr,
         .openLVar = "FSS_EXX_DOOR_CARGO_FWD_OPEN", .reaffirmTicks = kCargoDoorReaffirmTicks},
        {.door = GsxDoor::AftCargo, .reqLVar = "FSS_GNDSVC_CARGO_AFT_REQ", .ackLVar = nullptr,
         .openLVar = "FSS_EXX_DOOR_CARGO_AFT_OPEN", .reaffirmTicks = kCargoDoorReaffirmTicks},
        {.door = std::nullopt, .reqLVar = kMainDeckReqLVar, .ackLVar = nullptr,
         .openLVar = kMainDeckOpenLVar, .reaffirmTicks = kCargoDoorReaffirmTicks}
    }};

    constexpr std::size_t kMainDeckIndex = kDoorSlots.size() - 1;

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

RuleVerdict FssEJetDoorsFollowGsxRule::Evaluate(const RuleContext&)
{
    return RuleVerdict::Pass();
}

void FssEJetDoorsFollowGsxRule::Act(const RuleContext&, VariableWriter& writer)
{
    const int closeAllRequests = aircraft_->CloseAllRequests();
    const bool closeAllPending = closeAllRequests != servedCloseAllRequests_;

    if (closeAllPending)
    {
        doors_->CloseAll([this](const GsxDoor door, const bool open) { SetDesired(door, open); });
        servedCloseAllRequests_ = closeAllRequests;
    }
    else
    {
        doors_->Sync([this](const GsxDoor door, const bool open) { SetDesired(door, open); });
    }

    if (cargoVariant_)
    {
        SetMainDeckDesired(closeAllPending);
    }

    const std::size_t slotCount = cargoVariant_ ? kDoorSlots.size() : kMainDeckIndex;

    for (std::size_t index = 0; index < slotCount; ++index)
    {
        ReconcileSlot(index, writer);
    }
}

void FssEJetDoorsFollowGsxRule::SetDesired(const GsxDoor door, const bool open)
{
    if (cargoVariant_ && IsDoorHiddenFromTheFreighter(door))
    {
        return;
    }

    const auto match = std::ranges::find(kDoorSlots, std::optional<GsxDoor>{door}, &FssEJetDoorSlot::door);
    if (match == kDoorSlots.end())
    {
        return;
    }

    const auto index = static_cast<std::size_t>(std::distance(kDoorSlots.begin(), match));
    states_[index].desired = open;
}

void FssEJetDoorsFollowGsxRule::SetMainDeckDesired(const bool closeAllPending)
{
    const bool wantOpen = !closeAllPending && IsMainLoaderWaitingForTheDeck();
    std::optional<bool>& desired = states_[kMainDeckIndex].desired;

    if (wantOpen || desired.has_value())
    {
        desired = wantOpen;
    }
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

        if (state.attempts >= kMaxReaffirms)
        {
            LOG_INFO("FSS E-Jet door request %s reaffirmed for the last time: the aircraft has not confirmed it %s",
                     slot.reqLVar, wantOpen ? "open" : "closed");
        }
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
    probe::Line(probe::Channel::Writes, QStringLiteral("write door req=%1 open=%2").arg(QLatin1String(slot.reqLVar)).arg(open ? 1 : 0));
    writer.SetLVar(slot.reqLVar, open ? kDoorOpen : kDoorClosed);

    LOG_INFO("FSS E-Jet door commanded via %s: %s", slot.reqLVar, open ? "open" : "closed");
}

bool FssEJetDoorsFollowGsxRule::IsMainLoaderWaitingForTheDeck() const
{
    return gsx::states::IsLoaderServingTheDoor(doors_->VehicleState(gsx::lvars::kBaggageLoaderMainState, 0.0));
}


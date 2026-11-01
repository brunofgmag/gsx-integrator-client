#include "FssEJetDoorsFollowGsxRule.h"

#include <algorithm>
#include <cstddef>
#include <string>
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
    constexpr int kAckOpening = 1;
    constexpr int kAckClosing = 0;

    constexpr auto kMainDeckDesiredMemory = "fssEJet.mainDeckDesired";

    constexpr auto kMainDeckReqLVar = "FSS_GNDSVC_CARGO_MAIN_REQ";
    constexpr auto kMainDeckOpenLVar = "FSS_EXX_DOOR_CARGO_MAIN_OPEN";

    constexpr std::array<FssEJetDoorSlot, kFssEJetDoorSlotCount> kDoorSlots{
        {
            {.door = GsxDoor::FwdPax, .reqLVar = "FSS_GNDSVC_MAINDOOR_FWD_L_REQ", .ackLVar = "FSS_FLTCREW_MAINDOOR_FWD_L_REQ",
             .openLVar = "FSS_EXX_DOOR_FWD_L_OPEN", .reaffirmTicks = kPassengerDoorReaffirmTicks,
            },
            {.door = GsxDoor::AftPax, .reqLVar = "FSS_GNDSVC_MAINDOOR_AFT_L_REQ", .ackLVar = "FSS_FLTCREW_MAINDOOR_AFT_L_REQ",
             .openLVar = "FSS_EXX_DOOR_AFT_L_OPEN", .reaffirmTicks = kPassengerDoorReaffirmTicks,
            },
            {.door = GsxDoor::FwdCatering, .reqLVar = "FSS_GNDSVC_MAINDOOR_FWD_R_REQ", .ackLVar = "FSS_FLTCREW_MAINDOOR_FWD_R_REQ",
             .openLVar = "FSS_EXX_DOOR_FWD_R_OPEN", .reaffirmTicks = kPassengerDoorReaffirmTicks,
            },
            {.door = GsxDoor::AftCatering, .reqLVar = "FSS_GNDSVC_MAINDOOR_AFT_R_REQ", .ackLVar = "FSS_FLTCREW_MAINDOOR_AFT_R_REQ",
             .openLVar = "FSS_EXX_DOOR_AFT_R_OPEN", .reaffirmTicks = kPassengerDoorReaffirmTicks,
            },
            {.door = GsxDoor::FwdCargo, .reqLVar = "FSS_GNDSVC_CARGO_FWD_REQ", .ackLVar = nullptr,
             .openLVar = "FSS_EXX_DOOR_CARGO_FWD_OPEN", .reaffirmTicks = kCargoDoorReaffirmTicks,
            },
            {.door = GsxDoor::AftCargo, .reqLVar = "FSS_GNDSVC_CARGO_AFT_REQ", .ackLVar = nullptr,
             .openLVar = "FSS_EXX_DOOR_CARGO_AFT_OPEN", .reaffirmTicks = kCargoDoorReaffirmTicks,
            },
            {.door = std::nullopt, .reqLVar = kMainDeckReqLVar, .ackLVar = nullptr,
             .openLVar = kMainDeckOpenLVar, .reaffirmTicks = kCargoDoorReaffirmTicks,
            },
        },
    };

    constexpr std::size_t kMainDeckIndex = kDoorSlots.size() - 1;

    DoorPosition WantedPosition(const bool wantOpen)
    {
        return wantOpen ? DoorPosition::Open : DoorPosition::Closed;
    }

    DoorPosition CommandedPosition(const std::optional<bool>& commanded)
    {
        return commanded.has_value() ? WantedPosition(*commanded) : DoorPosition::NotRead;
    }

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
        doors_->CloseAll([this](const GsxDoor door, const bool open) { SetDesired(door, open, FssEJetDoorOrder::Absolute); });
        servedCloseAllRequests_ = closeAllRequests;
    }
    else
    {
        doors_->Sync([this](const GsxDoor door, const bool open) { SetDesired(door, open, FssEJetDoorOrder::Follow); });
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

void FssEJetDoorsFollowGsxRule::SetDesired(const GsxDoor door, const bool open, const FssEJetDoorOrder order)
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
    states_[index].order = order;
}

void FssEJetDoorsFollowGsxRule::AppendMemory(MemoryBag& memory) const
{
    const std::optional<bool>& desired = states_[kMainDeckIndex].desired;
    if (desired.has_value())
    {
        memory.PutFlag(kMainDeckDesiredMemory, *desired);
    }
}

void FssEJetDoorsFollowGsxRule::RestoreMemory(const MemoryBag& memory)
{
    if (!cargoVariant_ || memory.Text(kMainDeckDesiredMemory, {}).empty())
    {
        return;
    }

    SlotState& state = states_[kMainDeckIndex];
    state.desired = memory.Flag(kMainDeckDesiredMemory, false);
    state.commanded = state.desired;
    state.attempts = kMaxReaffirms;
    mainDeckRestoredOpen_ = *state.desired;
}

void FssEJetDoorsFollowGsxRule::ReclaimAnOpenMainDeck()
{
    reclaimsTheMainDeck_ = cargoVariant_;
}

void FssEJetDoorsFollowGsxRule::SetMainDeckDesired(const bool closeAllPending)
{
    if (!closeAllPending && HoldsTheRestoredMainDeckOpen())
    {
        return;
    }

    const bool wantOpen = !closeAllPending && IsMainLoaderWaitingForTheDeck();
    const bool claimsTheOpenDeck = ClaimsTheOpenMainDeck(closeAllPending);
    SlotState& state = states_[kMainDeckIndex];

    if (wantOpen || claimsTheOpenDeck || state.desired.has_value())
    {
        state.desired = wantOpen;
        state.order = closeAllPending ? FssEJetDoorOrder::Absolute : FssEJetDoorOrder::Follow;
    }

    if (claimsTheOpenDeck && !wantOpen)
    {
        state.commanded.reset();
    }
}

bool FssEJetDoorsFollowGsxRule::HoldsTheRestoredMainDeckOpen()
{
    if (!mainDeckRestoredOpen_)
    {
        return false;
    }

    if (!variables_->HasReceivedLVar(gsx::lvars::kBaggageLoaderMainState))
    {
        return true;
    }

    mainDeckRestoredOpen_ = false;

    return false;
}

bool FssEJetDoorsFollowGsxRule::ClaimsTheOpenMainDeck(const bool closeAllPending)
{
    closesTheMainDeck_ = closesTheMainDeck_ || closeAllPending;

    if ((!closesTheMainDeck_ && !reclaimsTheMainDeck_) || !HasTheReadingsTheClaimNeeds())
    {
        return false;
    }

    closesTheMainDeck_ = false;
    reclaimsTheMainDeck_ = false;

    return variables_->GetLVar(kMainDeckOpenLVar, 0.0) > 0.0;
}

bool FssEJetDoorsFollowGsxRule::HasTheReadingsTheClaimNeeds() const
{
    return variables_->HasReceivedLVar(kMainDeckOpenLVar)
        && (closesTheMainDeck_ || variables_->HasReceivedLVar(gsx::lvars::kBaggageLoaderMainState));
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

    AgeTransit(slot, state);

    if (state.commanded != state.desired)
    {
        RequestTheNewTarget(slot, state, writer);

        return;
    }

    if (PositionOf(slot) == WantedPosition(wantOpen))
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

void FssEJetDoorsFollowGsxRule::RequestTheNewTarget(const FssEJetDoorSlot& slot, SlotState& state, VariableWriter& writer)
{
    const bool wantOpen = state.desired == std::optional(true);
    const DoorPosition position = JudgedPositionOf(slot, state);
    if (position == DoorPosition::Pending && state.order == FssEJetDoorOrder::Follow)
    {
        return;
    }

    state.commanded = state.desired;
    state.order = FssEJetDoorOrder::Follow;
    state.ticksSinceCommand = 0;
    state.attempts = 0;

    if (position != WantedPosition(wantOpen))
    {
        WriteRequest(slot, wantOpen, writer);
        state.transit.Begin();
    }
}

DoorPosition FssEJetDoorsFollowGsxRule::JudgedPositionOf(const FssEJetDoorSlot& slot, const SlotState& state) const
{
    const DoorPosition position = PositionOf(slot);

    return state.transit.Holds(position, CommandedPosition(state.commanded)) ? DoorPosition::Pending : position;
}

void FssEJetDoorsFollowGsxRule::AgeTransit(const FssEJetDoorSlot& slot, SlotState& state) const
{
    if (!state.transit.IsRunning())
    {
        return;
    }

    const bool expiredUnobeyed = state.transit.Age(PositionOf(slot), CommandedPosition(state.commanded));
    if (!expiredUnobeyed)
    {
        return;
    }

    probe::Line(probe::Channel::Writes,
                QStringLiteral("hold  door req=%1 transit over after %2 ticks, reading believed")
                .arg(QLatin1String(slot.reqLVar)).arg(DoorTransit::kBoundTicks));
    LOG_INFO("FSS E-Jet door request %s did not show in %d ticks: its reading is believed from now on",
             slot.reqLVar, DoorTransit::kBoundTicks);
}

DoorPosition FssEJetDoorsFollowGsxRule::PositionOf(const FssEJetDoorSlot& slot) const
{
    const bool hasAck = slot.ackLVar != nullptr;
    const char* const lVar = hasAck ? slot.ackLVar : slot.openLVar;
    if (!variables_->HasReceivedLVar(lVar))
    {
        return DoorPosition::Pending;
    }

    const double reading = variables_->GetLVar(lVar, 0.0);
    if (!hasAck)
    {
        return reading > 0.0 ? DoorPosition::Open : DoorPosition::Closed;
    }

    switch (static_cast<int>(reading) % kAckPhaseDivisor)
    {
    case kAckOpening:
        return DoorPosition::Open;
    case kAckClosing:
        return DoorPosition::Closed;
    default:
        return DoorPosition::NotRead;
    }
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


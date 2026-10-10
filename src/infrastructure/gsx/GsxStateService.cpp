#include "GsxStateService.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <ranges>
#include <string_view>
#include <utility>

#include "GsxLVars.h"
#include "../logging/LogMacros.h"
#include "../../infrastructure/simvars/VariableGateway.h"

using namespace gsx::lvars;

namespace
{
    constexpr auto kNoPushbackVerdict = "no pushback";
    constexpr auto kSimOnGround = "SIM ON GROUND";
    constexpr auto kBoolUnit = "Bool";
    constexpr auto kGroundVelocity = "GROUND VELOCITY";
    constexpr auto kKnotsUnit = "Knots";
    constexpr auto kOperateStairsService = "OperateStairs";

    constexpr double kAccessStillEvaluated = 0.0;
    constexpr double kAccessNotOffered = 2.0;
    constexpr double kAccessInPlace = 5.0;

    constexpr double kAutomationFlagNotYetRead = 1.0;

    constexpr double kPushbackUnderway = 5.0;
    constexpr double kPushbackWaitingForEngines = 8.0;
    constexpr double kPushbackFinished = 11.0;

    constexpr std::array kServicesThatCancelOnReposition = {
        GsxState::Refueling,
        GsxState::Boarding,
        GsxState::Deboarding,
    };

    constexpr std::array kBaggageLoaders = {
        std::pair{kBaggageLoaderMainState, CargoLoader::MainDeck},
        std::pair{kBaggageLoaderRearState, CargoLoader::Rear},
        std::pair{kBaggageLoaderFrontState, CargoLoader::Front},
    };

    constexpr std::array kPassengerStairsVehicles = {
        kPassengerStairsFrontState,
        kPassengerStairsMiddleState,
        kPassengerStairsRearState,
    };

    constexpr std::array kResumeReadings = {
        kCouatlStarted,
        kRefuelingState,
        kBoardingState,
        kPushbackVehicleState,
        kDeboardingState,
        kDeiceState,
        kPushbackStatus,
    };

    constexpr auto kServicePrefix = "gsx.service.";
    constexpr auto kPrefixSeparator = ".";
    constexpr auto kBoardedPassengersPrefix = "gsx.boardedPassengers.";
    constexpr auto kDeboardedPassengersPrefix = "gsx.deboardedPassengers.";
    constexpr auto kBoardingCargoPrefix = "gsx.boardingCargo.";
    constexpr auto kDeboardingCargoPrefix = "gsx.deboardingCargo.";
    constexpr auto kFuelAndPayloadTakenOverEntry = "gsx.fuelAndPayloadTakenOver";
    constexpr auto kGpuConnectedSeenClearEntry = "gsx.gpuConnectedSeenClear";

    constexpr auto kCompletedField = "completed";
    constexpr auto kStatusField = "status";
    constexpr auto kCouatlDiedField = "couatlDiedDuringRun";
    constexpr auto kLastField = "last";
    constexpr auto kTotalField = "total";
    constexpr auto kCountingField = "counting";
    constexpr auto kMovedField = "moved";
    constexpr auto kGrownField = "grown";

    constexpr double kLargestSavedCount = 1000000.0;
    constexpr double kLargestSavedPercent = 100.0;

    std::string EntryName(const std::string_view prefix, const std::string_view field)
    {
        return std::string(prefix).append(field);
    }

    constexpr auto kUnknownServiceName = "unknown";

    const char* ServiceName(const GsxState gsxState)
    {
        switch (gsxState)
        {
        case GsxState::Refueling: return "refueling";
        case GsxState::Boarding: return "boarding";
        case GsxState::Pushback: return "pushback";
        case GsxState::Deboarding: return "deboarding";
        case GsxState::Deice: return "deice";
        }

        return kUnknownServiceName;
    }

    std::string ServicePrefix(const GsxState gsxState)
    {
        return EntryName(kServicePrefix, ServiceName(gsxState)).append(kPrefixSeparator);
    }

    GsxStateStatus StatusFromNumber(const double number)
    {
        const bool known = number >= static_cast<double>(GsxStateStatus::Unavailable)
            && number <= static_cast<double>(GsxStateStatus::Completing);

        return known ? static_cast<GsxStateStatus>(static_cast<int>(number)) : GsxStateStatus::Unavailable;
    }

    int CountFromNumber(const double number)
    {
        return static_cast<int>(std::clamp(number, 0.0, kLargestSavedCount));
    }

    double PercentFromNumber(const double number)
    {
        return std::clamp(number, 0.0, kLargestSavedPercent);
    }

    bool IsRequestedOrUnderway(const GsxStateStatus status)
    {
        return status == GsxStateStatus::Requested || status == GsxStateStatus::Active
            || status == GsxStateStatus::Completing;
    }

    bool IsIdle(const GsxStateStatus status)
    {
        return status == GsxStateStatus::Callable || status == GsxStateStatus::Bypassed;
    }

    bool EqualsFold(const std::string& lhs, const std::string_view rhs)
    {
        return std::ranges::equal(lhs, rhs, [](const char x, const char y)
        {
            return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
        });
    }

    const char* StateLVarName(const GsxState gsxState)
    {
        switch (gsxState)
        {
        case GsxState::Refueling: return kRefuelingState;
        case GsxState::Boarding: return kBoardingState;
        case GsxState::Pushback: return kPushbackVehicleState;
        case GsxState::Deboarding: return kDeboardingState;
        case GsxState::Deice: return kDeiceState;
        default: return nullptr;
        }
    }

    bool EndsWithoutCompleted(const GsxState gsxState)
    {
        return gsxState == GsxState::Pushback || gsxState == GsxState::Deice;
    }

    bool RemoteApiListsCallable(const GsxRemoteState* remote, const char* serviceId)
    {
        if (remote == nullptr || !remote->connected)
        {
            return false;
        }

        const GsxRemoteService* service = FindService(*remote, serviceId);

        return service != nullptr
            && service->stateRaw == static_cast<int>(GsxStateStatus::Callable)
            && service->canTrigger;
    }
}

GsxStateService::GsxStateService(VariableGateway* variableGateway, const GsxRemoteState* remoteState)
    : varManager_(variableGateway), remote_(remoteState),
      states_{
          {GsxState::Refueling, {}},
          {GsxState::Boarding, {}},
          {GsxState::Pushback, {}},
          {GsxState::Deboarding, {}},
          {GsxState::Deice, {}},
      }
{
}

void GsxStateService::Reset()
{
    boarding_ = {};
    deboarding_ = {};
    boardingCargo_ = {};
    deboardingCargo_ = {};
    fuelAndPayloadTakenOver_ = false;
    gpuConnectedSeenClear_ = false;
    gsxDownSinceLastObserve_ = false;

    for (auto& track : states_ | std::views::values)
    {
        track = {};
    }
}

MemoryBag GsxStateService::TakeMemory() const
{
    MemoryBag memory;

    for (const auto& [gsxState, track] : states_)
    {
        track.Save(memory, ServicePrefix(gsxState));
    }

    boarding_.Save(memory, kBoardedPassengersPrefix);
    deboarding_.Save(memory, kDeboardedPassengersPrefix);
    boardingCargo_.Save(memory, kBoardingCargoPrefix);
    deboardingCargo_.Save(memory, kDeboardingCargoPrefix);
    memory.PutFlag(kFuelAndPayloadTakenOverEntry, fuelAndPayloadTakenOver_);
    memory.PutFlag(kGpuConnectedSeenClearEntry, gpuConnectedSeenClear_);

    return memory;
}

void GsxStateService::RestoreMemory(const MemoryBag& memory, const bool gsxRestartedSinceSave)
{
    Reset();

    for (auto& [gsxState, track] : states_)
    {
        track.Load(memory, ServicePrefix(gsxState));
        track.resumption = gsxRestartedSinceSave ? StateTrack::Resumption::AfterGsxRestart
                                                 : StateTrack::Resumption::OnSameGsx;
    }

    boarding_.Load(memory, kBoardedPassengersPrefix);
    deboarding_.Load(memory, kDeboardedPassengersPrefix);
    boardingCargo_.Load(memory, kBoardingCargoPrefix);
    deboardingCargo_.Load(memory, kDeboardingCargoPrefix);
    fuelAndPayloadTakenOver_ = memory.Flag(kFuelAndPayloadTakenOverEntry, false);
    gpuConnectedSeenClear_ = memory.Flag(kGpuConnectedSeenClearEntry, false);
}

bool GsxStateService::HaveResumeReadingsArrived() const
{
    const auto missing = std::ranges::count_if(kResumeReadings, [this](const char* lVar)
    {
        return !varManager_->HasReceivedLVar(lVar);
    });

    return missing == 0;
}

void GsxStateService::StateTrack::Save(MemoryBag& memory, const std::string& prefix) const
{
    memory.PutFlag(EntryName(prefix, kCompletedField), completed);
    memory.PutNumber(EntryName(prefix, kStatusField), static_cast<double>(status));
    memory.PutFlag(EntryName(prefix, kCouatlDiedField), couatlDiedDuringRun);
}

void GsxStateService::StateTrack::Load(const MemoryBag& memory, const std::string& prefix)
{
    completed = memory.Flag(EntryName(prefix, kCompletedField), false);
    status = StatusFromNumber(memory.Number(EntryName(prefix, kStatusField), 0.0));
    couatlDiedDuringRun = memory.Flag(EntryName(prefix, kCouatlDiedField), false);
}

bool GsxStateService::StateTrack::HadStarted() const
{
    return status == GsxStateStatus::Active || status == GsxStateStatus::Completing
        || (resumption != Resumption::None && status == GsxStateStatus::Requested);
}

bool GsxStateService::StateTrack::WasInterruptedByGsxRestart() const
{
    return resumption == Resumption::AfterGsxRestart && IsRequestedOrUnderway(status);
}

bool GsxStateService::StateTrack::ReturnedToIdle(const GsxStateStatus reading, const bool endsWithoutCompleted) const
{
    if (!IsIdle(reading))
    {
        return false;
    }

    if (endsWithoutCompleted)
    {
        return resumption == Resumption::None && status == GsxStateStatus::Active;
    }

    return HadStarted() && !couatlDiedDuringRun;
}

void GsxStateService::OnTurnaroundTurned()
{
    boarding_ = {};
    deboarding_ = {};
    boardingCargo_ = {};
    deboardingCargo_ = {};

    for (auto& track : states_ | std::views::values)
    {
        track.completed = false;
        track.couatlDiedDuringRun = false;
    }
}

bool GsxStateService::IsAvailable() const
{
    return varManager_->GetLVar(kCouatlStarted) >= 1.0;
}

void GsxStateService::Observe()
{
    const LVarSpan couatlStarted = varManager_->ConsumeLVarSpan(kCouatlStarted);
    gsxDownSinceLastObserve_ = couatlStarted.received && couatlStarted.min < 1.0;

    for (const GsxState gsxState : states_ | std::views::keys)
    {
        ObserveState(gsxState);
    }

    ObserveGpuConnected();
}

void GsxStateService::ObserveGpuConnected()
{
    if (!varManager_->HasReceivedLVar(kGpuConnected))
    {
        return;
    }

    gpuConnectedSeenClear_ = gpuConnectedSeenClear_ || varManager_->GetLVar(kGpuConnected) != 1.0;
}

GsxStateStatus GsxStateService::GetStateStatus(const GsxState gsxState) const
{
    const char* stateLVar = StateLVarName(gsxState);
    if (stateLVar == nullptr)
    {
        return GsxStateStatus::Unavailable;
    }

    return static_cast<GsxStateStatus>(varManager_->GetLVar(stateLVar));
}

bool GsxStateService::WasStateCompleted(const GsxState gsxState) const
{
    return states_.at(gsxState).completed;
}

bool GsxStateService::IsFuelHoseConnected() const
{
    return varManager_->GetLVar(kFuelHoseConnected) >= 1.0;
}

double GsxStateService::GetRefuelCounterGallons() const
{
    return std::max(varManager_->GetLVar(kFuelCounter),
                    varManager_->GetLVar(kFuelCounterMax));
}

bool GsxStateService::HasPushbackStarted() const
{
    return varManager_->GetLVar(kPushbackStatus) >= kPushbackUnderway;
}

bool GsxStateService::IsPushbackFinished() const
{
    return varManager_->GetLVar(kPushbackStatus) == kPushbackFinished;
}

bool GsxStateService::IsWaitingForEngines() const
{
    return varManager_->GetLVar(kPushbackStatus) == kPushbackWaitingForEngines;
}

bool GsxStateService::IsRepositioning() const
{
    return varManager_->GetLVar(kRepositioning) == 1.0;
}

std::optional<bool> GsxStateService::HasServiceUnderway() const
{
    const bool allArrived = std::ranges::all_of(kServicesThatCancelOnReposition, [this](const GsxState gsxState)
    {
        return varManager_->HasReceivedLVar(StateLVarName(gsxState));
    });
    if (!allArrived)
    {
        return std::nullopt;
    }

    return std::ranges::any_of(kServicesThatCancelOnReposition, [this](const GsxState gsxState)
    {
        return IsRequestedOrUnderway(GetStateStatus(gsxState));
    });
}

int GsxStateService::GetPlannedPassengers() const
{
    return static_cast<int>(varManager_->GetLVar(kMaxPassengers));
}

int GsxStateService::GetBoardedPassengers()
{
    return ReadPassengers(boarding_, GsxState::Boarding, kNumPassengersBoardingTotal);
}

int GsxStateService::GetDeboardedPassengers()
{
    return ReadPassengers(deboarding_, GsxState::Deboarding, kNumPassengersDeboardingTotal);
}

int GsxStateService::ReadPassengers(PassengerCounter& counter, const GsxState gsxState, const char* counterLVar)
{
    if (!varManager_->HasReceivedLVar(counterLVar))
    {
        return counter.Reported();
    }

    const bool active = GetStateStatus(gsxState) == GsxStateStatus::Active;

    return counter.Update(static_cast<int>(varManager_->GetLVar(counterLVar)), active,
                          FoundServiceUnderway(gsxState));
}

bool GsxStateService::FoundServiceUnderway(const GsxState gsxState) const
{
    return states_.at(gsxState).foundUnderway;
}

int GsxStateService::PassengerCounter::Update(const int current, const bool active, const bool foundUnderway)
{
    if (!counting)
    {
        if (!active)
        {
            return 0;
        }

        counting = true;
        last = current;

        if (foundUnderway)
        {
            moved = true;
            grown = current > 0;

            return current;
        }

        return 0;
    }

    if (!moved)
    {
        if (current == last)
        {
            return 0;
        }

        moved = true;
    }

    if (current < last)
    {
        if (grown)
        {
            total += last;
        }

        grown = current > 0;
    }
    else if (current > last)
    {
        grown = true;
    }

    last = current;

    return total + current;
}

int GsxStateService::PassengerCounter::Reported() const
{
    return moved ? total + last : 0;
}

void GsxStateService::PassengerCounter::Save(MemoryBag& memory, const std::string& prefix) const
{
    memory.PutNumber(EntryName(prefix, kLastField), last);
    memory.PutNumber(EntryName(prefix, kTotalField), total);
    memory.PutFlag(EntryName(prefix, kCountingField), counting);
    memory.PutFlag(EntryName(prefix, kMovedField), moved);
    memory.PutFlag(EntryName(prefix, kGrownField), grown);
}

void GsxStateService::PassengerCounter::Load(const MemoryBag& memory, const std::string& prefix)
{
    last = CountFromNumber(memory.Number(EntryName(prefix, kLastField), 0.0));
    total = CountFromNumber(memory.Number(EntryName(prefix, kTotalField), 0.0));
    counting = memory.Flag(EntryName(prefix, kCountingField), false);
    moved = memory.Flag(EntryName(prefix, kMovedField), false);
    grown = memory.Flag(EntryName(prefix, kGrownField), false);
}

double GsxStateService::GetBoardingCargoPercent()
{
    return ReadCargoPercent(boardingCargo_, GsxState::Boarding, kBoardingCargoPercent);
}

double GsxStateService::ReadCargoPercent(CargoPercentReading& reading, const GsxState gsxState,
                                         const char* percentLVar)
{
    if (!varManager_->HasReceivedLVar(percentLVar))
    {
        return reading.Reported();
    }

    const bool active = GetStateStatus(gsxState) == GsxStateStatus::Active;

    return reading.Update(varManager_->GetLVar(percentLVar), active, FoundServiceUnderway(gsxState));
}

double GsxStateService::CargoPercentReading::Update(const double current, const bool active,
                                                    const bool foundUnderway)
{
    if (!counting)
    {
        if (!active)
        {
            return 0.0;
        }

        counting = true;
        first = current;
        last = current;
        moved = foundUnderway;

        return Reported();
    }

    last = current;
    moved = moved || current != first;

    return Reported();
}

double GsxStateService::CargoPercentReading::Reported() const
{
    return moved ? last : 0.0;
}

void GsxStateService::CargoPercentReading::Save(MemoryBag& memory, const std::string& prefix) const
{
    memory.PutNumber(EntryName(prefix, kLastField), last);
    memory.PutFlag(EntryName(prefix, kCountingField), counting);
    memory.PutFlag(EntryName(prefix, kMovedField), moved);
}

void GsxStateService::CargoPercentReading::Load(const MemoryBag& memory, const std::string& prefix)
{
    last = PercentFromNumber(memory.Number(EntryName(prefix, kLastField), 0.0));
    first = last;
    counting = memory.Flag(EntryName(prefix, kCountingField), false);
    moved = memory.Flag(EntryName(prefix, kMovedField), false);
}

bool GsxStateService::IsLoadingCargo() const
{
    return varManager_->GetLVar(kBoardingCargo) == 1.0;
}

CargoLoader GsxStateService::GetLoaderWaitingForDoor() const
{
    for (const auto& [state, loader] : kBaggageLoaders)
    {
        if (varManager_->GetLVar(state) == gsx::states::kLoaderWaitingForDoor)
        {
            return loader;
        }
    }

    return CargoLoader::None;
}

bool GsxStateService::IsALoaderAtAHold() const
{
    return std::ranges::any_of(kBaggageLoaders, [this](const auto& loader)
    {
        const double state = varManager_->GetLVar(loader.first);

        return state == gsx::states::kLoaderInPosition || state == gsx::states::kLoaderLoading;
    });
}

double GsxStateService::GetDeboardingCargoPercent()
{
    return ReadCargoPercent(deboardingCargo_, GsxState::Deboarding, kDeboardingCargoPercent);
}

bool GsxStateService::AreStairsInPlace() const
{
    return varManager_->GetLVar(kStairs) == kAccessInPlace;
}

bool GsxStateService::IsJetwayInPlace() const
{
    return varManager_->GetLVar(kJetway) == kAccessInPlace;
}

GroundPowerStatus GsxStateService::GetGpuStatus() const
{
    if (const GsxRemoteService* service =
            remote_ != nullptr ? FindService(*remote_, gsx::services::Id(GroundService::Gpu)) : nullptr;
        service != nullptr)
    {
        return service->stateRaw == static_cast<int>(GsxStateStatus::Active)
            ? GroundPowerStatus::Connected
            : GroundPowerStatus::Disconnected;
    }

    if (!varManager_->HasReceivedLVar(kGpuState))
    {
        return GroundPowerStatus::Unknown;
    }

    const double state = varManager_->GetLVar(kGpuState);

    const bool connected = state == static_cast<double>(GsxStateStatus::Active)
        || (gpuConnectedSeenClear_ && varManager_->GetLVar(kGpuConnected) == 1.0);

    return connected ? GroundPowerStatus::Connected : GroundPowerStatus::Disconnected;
}

bool GsxStateService::IsServiceInProgress(const GroundService service) const
{
    if (remote_ == nullptr)
    {
        return false;
    }

    const char* id = gsx::services::Id(service);
    if (id == nullptr)
    {
        return false;
    }

    const GsxRemoteService* svc = FindService(*remote_, id);
    if (svc == nullptr)
    {
        return false;
    }

    return svc->stateRaw == static_cast<int>(GsxStateStatus::Requested)
        || svc->stateRaw == static_cast<int>(GsxStateStatus::Active);
}

bool GsxStateService::OffersPushback() const
{
    if (remote_ == nullptr)
    {
        return true;
    }

    return std::ranges::none_of(remote_->apronVerdict, [](const std::string& descriptor)
    {
        return EqualsFold(descriptor, kNoPushbackVerdict);
    });
}

bool GsxStateService::IsRemoteApiConnected() const
{
    return remote_ != nullptr && remote_->connected;
}

bool GsxStateService::WasGsxDownSinceLastObserve() const
{
    return gsxDownSinceLastObserve_;
}

bool GsxStateService::AreStairsAvailable() const
{
    const double state = varManager_->GetLVar(kStairs, kAccessStillEvaluated);

    if (state == kAccessStillEvaluated)
    {
        const bool noJetwayHere = varManager_->GetLVar(kJetway, kAccessStillEvaluated) == kAccessNotOffered;

        return noJetwayHere && RemoteApiListsCallable(remote_, kOperateStairsService);
    }

    return state != kAccessNotOffered;
}

bool GsxStateService::IsJetwayAvailable() const
{
    const double state = varManager_->GetLVar(kJetway, kAccessStillEvaluated);

    return state != kAccessStillEvaluated && state != kAccessNotOffered;
}

bool GsxStateService::IsJetwayOrStairsOperating() const
{
    return varManager_->GetLVar(kJetway) == static_cast<double>(GsxStateStatus::Requested)
        || varManager_->GetLVar(kStairs) == static_cast<double>(GsxStateStatus::Requested);
}

bool GsxStateService::IsServiceVehicleActive() const
{
    return std::ranges::any_of(kPassengerStairsVehicles, [this](const char* vehicle)
    {
        return varManager_->GetLVar(vehicle, 0.0) >= gsx::states::kVehicleDispatched;
    });
}

bool GsxStateService::IsAircraftOnGround() const
{
    return varManager_->GetAVar(kSimOnGround, kBoolUnit, 1.0) == 1.0;
}

double GsxStateService::GetGroundSpeedKnots() const
{
    return varManager_->GetAVar(kGroundVelocity, kKnotsUnit, 0.0);
}

void GsxStateService::TakeOverFuelAndPayload()
{
    varManager_->SetLVar(kAutomationFuel, 0.0);
    varManager_->SetLVar(kAutomationPayload, 0.0);
    fuelAndPayloadTakenOver_ = true;

    LOG_INFO("Taking over fuel and payload insertion");
}

void GsxStateService::ReassertTakeovers() const
{
    if (!fuelAndPayloadTakenOver_)
    {
        return;
    }

    const bool mayBeRaised = varManager_->GetLVar(kAutomationFuel, kAutomationFlagNotYetRead) != 0.0
        || varManager_->GetLVar(kAutomationPayload, kAutomationFlagNotYetRead) != 0.0;
    if (!mayBeRaised)
    {
        return;
    }

    if (IsAutomationFlagRaised(kAutomationFuel) || IsAutomationFlagRaised(kAutomationPayload))
    {
        LOG_INFO("GSX automation flags reset by couatl; re-taking fuel and payload");
    }

    varManager_->SetLVar(kAutomationFuel, 0.0);
    varManager_->SetLVar(kAutomationPayload, 0.0);
}

bool GsxStateService::IsAutomationFlagRaised(const char* lVar) const
{
    return varManager_->HasReceivedLVar(lVar) && varManager_->GetLVar(lVar) != 0.0;
}

bool GsxStateService::IsSimbriefLoaded() const
{
    return varManager_->GetLVar(kSimbriefSuccess) >= 1.0;
}

int GsxStateService::GetServedSimbriefGeneration() const
{
    if (remote_ == nullptr)
    {
        return 0;
    }

    return remote_->simbriefGeneration;
}

std::string GsxStateService::GetSimbriefRefusal() const
{
    if (remote_ == nullptr || remote_->simbriefStatus != "error")
    {
        return {};
    }

    return remote_->simbriefError;
}

bool GsxStateService::IsGoodEngineStartConfirmationEnabled() const
{
    return varManager_->GetLVar(kGoodEngineStart, 1.0) >= 1.0;
}

void GsxStateService::ObserveState(const GsxState gsxState)
{
    const char* stateLVar = StateLVarName(gsxState);
    if (stateLVar == nullptr || !varManager_->HasReceivedLVar(stateLVar))
    {
        return;
    }

    const auto reading = static_cast<GsxStateStatus>(varManager_->GetLVar(stateLVar));
    StateTrack& track = states_.at(gsxState);
    const bool endsWithoutCompleted = EndsWithoutCompleted(gsxState);
    const bool isActive = reading == GsxStateStatus::Active;

    if (!track.firstReadingSeen)
    {
        track.firstReadingSeen = true;
        track.foundUnderway = isActive;
    }

    track.foundUnderway = track.foundUnderway && isActive;

    if (!endsWithoutCompleted && track.WasInterruptedByGsxRestart())
    {
        track.couatlDiedDuringRun = true;
    }

    if (gsxDownSinceLastObserve_)
    {
        track.couatlDiedDuringRun = true;
    }
    else if (isActive && track.status != GsxStateStatus::Active)
    {
        track.couatlDiedDuringRun = false;
    }

    track.completed = track.completed || reading == GsxStateStatus::Completed
        || track.ReturnedToIdle(reading, endsWithoutCompleted);
    track.status = reading;
    track.resumption = StateTrack::Resumption::None;
}

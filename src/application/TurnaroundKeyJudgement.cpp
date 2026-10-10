#include "TurnaroundKeyJudgement.h"

#include <string>

namespace
{
    bool DiffersWhenBothAreKnown(const std::string& saved, const std::string& live)
    {
        return !saved.empty() && !live.empty() && saved != live;
    }

    bool HasFlownSinceTheSave(const std::optional<TurnaroundPhase>& savedPhase)
    {
        return savedPhase.has_value()
            && *savedPhase >= TurnaroundPhase::WaitingDeparture
            && *savedPhase <= TurnaroundPhase::WaitingEngineShutdown;
    }

    bool IsGroundPhase(const std::optional<TurnaroundPhase>& savedPhase)
    {
        return savedPhase.has_value() && !HasFlownSinceTheSave(savedPhase);
    }

    bool IsAnotherStand(const TurnaroundKey& saved, const TurnaroundKey& live)
    {
        return DiffersWhenBothAreKnown(saved.airportIcao, live.airportIcao)
            || DiffersWhenBothAreKnown(saved.parkingName, live.parkingName);
    }

    bool IsStillWaitingForTheStand(const TurnaroundKey& saved, const TurnaroundKey& live,
                                   const std::optional<TurnaroundPhase>& savedPhase)
    {
        return IsGroundPhase(savedPhase) && !saved.parkingName.empty() && live.parkingName.empty();
    }
}

KeyVerdict TurnaroundKeyJudgement::Judge(const TurnaroundKey& saved,
                                         const TurnaroundKey& live,
                                         const std::optional<TurnaroundPhase> savedPhase)
{
    if (live.aircraftTitle.empty())
    {
        return KeyVerdict::NotYetJudgeable;
    }

    if (saved.aircraftTitle != live.aircraftTitle)
    {
        return KeyVerdict::Different;
    }

    if (live.aircraftId.empty())
    {
        return KeyVerdict::NotYetJudgeable;
    }

    if (saved.aircraftId != live.aircraftId)
    {
        return KeyVerdict::Different;
    }

    if (!HasFlownSinceTheSave(savedPhase))
    {
        if (IsAnotherStand(saved, live))
        {
            return KeyVerdict::Different;
        }

        if (IsStillWaitingForTheStand(saved, live, savedPhase))
        {
            return KeyVerdict::NotYetJudgeable;
        }
    }

    if (live.couatlId.empty())
    {
        return KeyVerdict::NotYetJudgeable;
    }

    return saved.couatlId == live.couatlId ? KeyVerdict::Same : KeyVerdict::OnlyCouatlDiffers;
}

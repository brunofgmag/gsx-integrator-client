#include "TurnaroundKeyJudgement.h"

#include <string>
#include <string_view>

namespace
{
    constexpr char kStandGroupSeparator = '|';

    struct Stand
    {
        std::string group;
        std::string name;
    };

    std::string Trimmed(const std::string_view text)
    {
        const std::size_t first = text.find_first_not_of(' ');
        if (first == std::string_view::npos)
        {
            return {};
        }

        return std::string(text.substr(first, text.find_last_not_of(' ') - first + 1));
    }

    Stand StandOf(const std::string_view parkingName)
    {
        const std::size_t separator = parkingName.rfind(kStandGroupSeparator);
        if (separator == std::string_view::npos)
        {
            return Stand{.group = {}, .name = Trimmed(parkingName)};
        }

        return Stand{.group = Trimmed(parkingName.substr(0, separator)),
                     .name = Trimmed(parkingName.substr(separator + 1))};
    }

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

    bool IsAnotherStand(const TurnaroundKey& saved, const TurnaroundKey& live)
    {
        return DiffersWhenBothAreKnown(saved.airportIcao, live.airportIcao)
            || (!saved.parkingName.empty() && !live.parkingName.empty()
                && !TurnaroundKeyJudgement::IsSameStand(saved.parkingName, live.parkingName));
    }

    bool IsStillWaitingForTheStand(const TurnaroundKey& saved, const TurnaroundKey& live)
    {
        return !saved.parkingName.empty() && live.parkingName.empty();
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

        if (IsStillWaitingForTheStand(saved, live))
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

bool TurnaroundKeyJudgement::IsSameStand(const std::string& saved, const std::string& live)
{
    const Stand savedStand = StandOf(saved);
    const Stand liveStand = StandOf(live);
    const bool groupsAgree = savedStand.group.empty() || liveStand.group.empty() || savedStand.group == liveStand.group;

    return groupsAgree && savedStand.name == liveStand.name;
}

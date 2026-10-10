#include "TurnaroundRestore.h"

#include <string_view>
#include "../ports/Aircraft.h"

namespace
{
    constexpr double kFullProgress = 100.0;

    constexpr bool DeboardingIsOver(const TurnaroundPhase phase)
    {
        return phase >= TurnaroundPhase::CabinServices;
    }

    double RebuiltLoadedFuelKg(const TurnaroundData& data, const Aircraft& aircraft)
    {
        const RefuelBy method = aircraft.GetRefuelMethod();

        if (method != RefuelBy::Gsx && data.refuelFinished)
        {
            return data.plannedFuelKg;
        }

        if (method == RefuelBy::Self && data.refuelBaselined)
        {
            return data.initialFuelKg;
        }

        return aircraft.GetCurrentFuelKg();
    }

    void RebuildFuelFigures(TurnaroundData& data, const Aircraft& aircraft)
    {
        data.loadedFuelKg = RebuiltLoadedFuelKg(data, aircraft);

        if (data.refuelFinished)
        {
            data.fuelProgress = kFullProgress;
        }
    }

    void RebuildBoardingFigures(TurnaroundData& data)
    {
        if (!data.boardingFinished)
        {
            return;
        }

        data.boardingProgress = kFullProgress;
        data.boardedPassengers = data.plannedPassengers;
        data.loadedZfwKg = data.plannedZfwKg;
    }

    void RebuildDeboardingFigures(TurnaroundData& data, const TurnaroundPhase phase)
    {
        if (!DeboardingIsOver(phase))
        {
            return;
        }

        data.deboardingProgress = kFullProgress;
        data.loadedZfwKg = data.initialZfwKg;
    }

    void TakeTheRepositionAlreadyAskedAsSeen(TurnaroundData& data, const TurnaroundPhase phase)
    {
        if (phase == TurnaroundPhase::RepositionAircraft && (data.repositionRequested || data.repositionAttempted))
        {
            data.repositionCompleted = true;
        }
    }

    void MarkThePushbackLostToTheRestart(TurnaroundData& data, const bool gsxRestartedSinceSave)
    {
        if (data.pushbackPending && gsxRestartedSinceSave)
        {
            data.pushbackLostToGsxRestart = true;
        }
    }
}

namespace turnaround
{
    TurnaroundData SavedFields(const TurnaroundData& data)
    {
        TurnaroundData saved;

        VisitFields([&data, &saved](const std::string_view, const auto field, const FieldRestore restore)
        {
            if (restore == FieldRestore::Raw)
            {
                field(saved) = field(data);
            }
        });

        return saved;
    }

    TurnaroundData RestoreTurnaroundData(const TurnaroundData& saved,
                                         const TurnaroundPhase phase,
                                         const Aircraft& aircraft,
                                         const bool gsxRestartedSinceSave)
    {
        TurnaroundData restored = SavedFields(saved);

        RebuildFuelFigures(restored, aircraft);
        RebuildBoardingFigures(restored);
        RebuildDeboardingFigures(restored, phase);
        TakeTheRepositionAlreadyAskedAsSeen(restored, phase);
        MarkThePushbackLostToTheRestart(restored, gsxRestartedSinceSave);

        return restored;
    }

    TurnaroundFacts BuildTurnaroundFacts(const TurnaroundPhase phase,
                                         const TurnaroundData& data,
                                         const bool gsxRestartedSinceSave)
    {
        TurnaroundFacts facts;
        facts.phase = phase;
        facts.loadingStarted = data.loadingStartNotified;
        facts.refuelFinished = data.refuelFinished;
        facts.boardingFinished = data.boardingFinished;
        facts.departureDoorsHeld = data.boardingFinished && !data.arrivalDoorsClosed;
        facts.passengerDoorsHeld = data.passengerDoorsHeldClosed && !data.arrivalDoorsClosed;
        facts.arrivalDoorsClosed = data.arrivalDoorsClosed;
        facts.gsxRestartedSinceSave = gsxRestartedSinceSave;
        facts.plannedFuelKg = data.plannedFuelKg;
        facts.plannedZfwKg = data.plannedZfwKg;
        facts.emptyZfwKg = data.initialZfwKg;
        facts.plannedPassengers = data.plannedPassengers;

        return facts;
    }
}

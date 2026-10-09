#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDDATA_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDDATA_H

#include <optional>
#include <set>
#include <string>

#include "../model/AutomationStatus.h"
#include "../model/CargoLoader.h"

struct CabinServiceProgress
{
    bool asked = false;
    bool requested = false;
    bool activeSeen = false;

    bool operator==(const CabinServiceProgress&) const = default;
};

struct TurnaroundData
{
    double plannedFuelKg = 0.0;
    double plannedZfwKg = 0.0;
    int plannedPassengers = 0;
    int boardedPassengers = 0;

    double loadedFuelKg = 0.0;
    double initialFuelKg = 0.0;
    double loadedZfwKg = 0.0;
    double initialZfwKg = 0.0;

    double fuelProgress = 0.0;
    double boardingProgress = 0.0;
    double deboardingProgress = 0.0;

    bool loadingConfirmed = false;
    bool loadingStartNotified = false;
    bool refuelBaselined = false;
    double refuelStallSampleKg = 0.0;
    int refuelStallTicks = 0;
    bool boardingBaselined = false;
    int boardingStallTicks = 0;
    int boardingCompletionAttempts = 0;
    CargoLoader loaderAwaitingDoor = CargoLoader::None;
    CargoLoader loaderHoldingBoarding = CargoLoader::None;
    int loaderDoorWaitTicks = 0;
    int loaderDoorWaitSeconds = 0;
    int cargoFlagAfterServiceTicks = 0;
    int loaderAtHoldAfterServiceTicks = 0;
    bool passengerDoorsHeldClosed = false;
    bool deboardingBaselined = false;
    bool refuelingRequested = false;
    int fuelRequestStallTicks = 0;
    bool fuelRequestStalled = false;
    bool fuelPlanOverCapacity = false;
    bool planOmitsCrew = false;
    double omittedCrewKg = 0.0;
    double operatingEmptyWithCrewKg = 0.0;
    bool fuelDidNotStay = false;
    bool fuelStayChecked = false;
    bool fuelStayDismissed = false;
    bool fuelTopUpStarted = false;
    double fuelShortfallKg = 0.0;
    double settledFuelKg = 0.0;
    EngineConfirmationBlock engineConfirmationBlock = EngineConfirmationBlock::None;
    bool engineConfirmationSent = false;
    bool engineWaitResumed = false;
    bool servicesStalled = false;
    bool serviceInterrupted = false;
    int servicesWaitSeconds = 0;
    int servicesOperatingTicks = 0;
    bool boardingRequested = false;
    bool boardingConfirmed = false;
    bool refuelFinished = false;
    bool boardingFinished = false;
    bool deboardingRequested = false;
    bool deboardingAwaitsGsx = false;
    bool pushbackRequested = false;
    bool pushbackPending = false;
    bool pushbackLostToGsxRestart = false;
    bool jetwayOrStairsRequested = false;
    bool jetwayOrStairsCompleted = false;
    int stairsInPlaceTicks = 0;
    bool gpuRequested = false;
    bool chocksPlaced = false;
    bool chocksRemoved = false;
    bool doorsClosed = false;
    bool ownGroundEquipmentCleared = false;
    bool arrivalGpuRequested = false;
    bool arrivalChocksPlaced = false;
    bool arrivalDoorsClosed = false;
    bool cateringAsked = false;
    bool cateringRequested = false;
    bool gpuDismissRequested = false;
    int cateringWaitIntervals = 0;
    CabinServiceProgress lavatory;
    CabinServiceProgress water;
    CabinServiceProgress cleaning;
    int cabinWaitIntervals = 0;
    bool flightPlanRequested = false;
    int flightPlanRequestTicks = 0;
    bool flightPlanRefused = false;
    bool latestFlightPlanRequested = false;
    int differingFlightPlanTicks = 0;
    std::optional<int> staleSimbriefGeneration;
    bool repositionRequested = false;
    bool repositionCompleted = false;

    int stateTickCount = 0;
    int ruleHoldTicks = 0;
    std::set<std::string> expiredRuleHolds;

    bool operator==(const TurnaroundData&) const = default;

    void Reset() { *this = {}; }
};

namespace turnaround
{
    enum class FieldRestore
    {
        Raw,
        Restart,
        Rebuilt,
        NotSaved,
    };

    template <typename Owner, typename Value>
    constexpr auto Field(Value Owner::* const member)
    {
        return [member](auto& owner) -> auto& { return owner.*member; };
    }

    template <typename Owner, typename Inner, typename Value>
    constexpr auto Field(Inner Owner::* const outer, Value Inner::* const inner)
    {
        return [outer, inner](auto& owner) -> auto& { return (owner.*outer).*inner; };
    }

    template <typename Visitor>
    void VisitFields(Visitor&& visit)
    {
        visit("plannedFuelKg", Field(&TurnaroundData::plannedFuelKg), FieldRestore::Raw);
        visit("plannedZfwKg", Field(&TurnaroundData::plannedZfwKg), FieldRestore::Raw);
        visit("plannedPassengers", Field(&TurnaroundData::plannedPassengers), FieldRestore::Raw);
        visit("boardedPassengers", Field(&TurnaroundData::boardedPassengers), FieldRestore::Rebuilt);
        visit("loadedFuelKg", Field(&TurnaroundData::loadedFuelKg), FieldRestore::Rebuilt);
        visit("initialFuelKg", Field(&TurnaroundData::initialFuelKg), FieldRestore::Raw);
        visit("loadedZfwKg", Field(&TurnaroundData::loadedZfwKg), FieldRestore::Rebuilt);
        visit("initialZfwKg", Field(&TurnaroundData::initialZfwKg), FieldRestore::Raw);
        visit("fuelProgress", Field(&TurnaroundData::fuelProgress), FieldRestore::Rebuilt);
        visit("boardingProgress", Field(&TurnaroundData::boardingProgress), FieldRestore::Rebuilt);
        visit("deboardingProgress", Field(&TurnaroundData::deboardingProgress), FieldRestore::Rebuilt);
        visit("loadingConfirmed", Field(&TurnaroundData::loadingConfirmed), FieldRestore::Raw);
        visit("loadingStartNotified", Field(&TurnaroundData::loadingStartNotified), FieldRestore::Raw);
        visit("refuelBaselined", Field(&TurnaroundData::refuelBaselined), FieldRestore::Raw);
        visit("refuelStallSampleKg", Field(&TurnaroundData::refuelStallSampleKg), FieldRestore::Restart);
        visit("refuelStallTicks", Field(&TurnaroundData::refuelStallTicks), FieldRestore::Restart);
        visit("boardingBaselined", Field(&TurnaroundData::boardingBaselined), FieldRestore::Raw);
        visit("boardingStallTicks", Field(&TurnaroundData::boardingStallTicks), FieldRestore::Restart);
        visit("boardingCompletionAttempts", Field(&TurnaroundData::boardingCompletionAttempts), FieldRestore::Raw);
        visit("loaderAwaitingDoor", Field(&TurnaroundData::loaderAwaitingDoor), FieldRestore::NotSaved);
        visit("loaderHoldingBoarding", Field(&TurnaroundData::loaderHoldingBoarding), FieldRestore::NotSaved);
        visit("loaderDoorWaitTicks", Field(&TurnaroundData::loaderDoorWaitTicks), FieldRestore::Restart);
        visit("loaderDoorWaitSeconds", Field(&TurnaroundData::loaderDoorWaitSeconds), FieldRestore::NotSaved);
        visit("cargoFlagAfterServiceTicks", Field(&TurnaroundData::cargoFlagAfterServiceTicks), FieldRestore::Restart);
        visit("loaderAtHoldAfterServiceTicks", Field(&TurnaroundData::loaderAtHoldAfterServiceTicks), FieldRestore::Restart);
        visit("passengerDoorsHeldClosed", Field(&TurnaroundData::passengerDoorsHeldClosed), FieldRestore::Raw);
        visit("deboardingBaselined", Field(&TurnaroundData::deboardingBaselined), FieldRestore::Raw);
        visit("refuelingRequested", Field(&TurnaroundData::refuelingRequested), FieldRestore::Raw);
        visit("fuelRequestStallTicks", Field(&TurnaroundData::fuelRequestStallTicks), FieldRestore::Restart);
        visit("fuelRequestStalled", Field(&TurnaroundData::fuelRequestStalled), FieldRestore::NotSaved);
        visit("fuelPlanOverCapacity", Field(&TurnaroundData::fuelPlanOverCapacity), FieldRestore::Raw);
        visit("planOmitsCrew", Field(&TurnaroundData::planOmitsCrew), FieldRestore::Raw);
        visit("omittedCrewKg", Field(&TurnaroundData::omittedCrewKg), FieldRestore::Raw);
        visit("operatingEmptyWithCrewKg", Field(&TurnaroundData::operatingEmptyWithCrewKg), FieldRestore::Raw);
        visit("fuelDidNotStay", Field(&TurnaroundData::fuelDidNotStay), FieldRestore::Raw);
        visit("fuelStayChecked", Field(&TurnaroundData::fuelStayChecked), FieldRestore::Raw);
        visit("fuelStayDismissed", Field(&TurnaroundData::fuelStayDismissed), FieldRestore::Raw);
        visit("fuelTopUpStarted", Field(&TurnaroundData::fuelTopUpStarted), FieldRestore::Raw);
        visit("fuelShortfallKg", Field(&TurnaroundData::fuelShortfallKg), FieldRestore::Raw);
        visit("settledFuelKg", Field(&TurnaroundData::settledFuelKg), FieldRestore::Raw);
        visit("engineConfirmationBlock", Field(&TurnaroundData::engineConfirmationBlock), FieldRestore::NotSaved);
        visit("engineConfirmationSent", Field(&TurnaroundData::engineConfirmationSent), FieldRestore::Raw);
        visit("engineWaitResumed", Field(&TurnaroundData::engineWaitResumed), FieldRestore::Raw);
        visit("servicesStalled", Field(&TurnaroundData::servicesStalled), FieldRestore::NotSaved);
        visit("serviceInterrupted", Field(&TurnaroundData::serviceInterrupted), FieldRestore::NotSaved);
        visit("servicesWaitSeconds", Field(&TurnaroundData::servicesWaitSeconds), FieldRestore::NotSaved);
        visit("servicesOperatingTicks", Field(&TurnaroundData::servicesOperatingTicks), FieldRestore::Restart);
        visit("boardingRequested", Field(&TurnaroundData::boardingRequested), FieldRestore::Raw);
        visit("boardingConfirmed", Field(&TurnaroundData::boardingConfirmed), FieldRestore::Raw);
        visit("refuelFinished", Field(&TurnaroundData::refuelFinished), FieldRestore::Raw);
        visit("boardingFinished", Field(&TurnaroundData::boardingFinished), FieldRestore::Raw);
        visit("deboardingRequested", Field(&TurnaroundData::deboardingRequested), FieldRestore::Raw);
        visit("deboardingAwaitsGsx", Field(&TurnaroundData::deboardingAwaitsGsx), FieldRestore::NotSaved);
        visit("pushbackRequested", Field(&TurnaroundData::pushbackRequested), FieldRestore::Raw);
        visit("pushbackPending", Field(&TurnaroundData::pushbackPending), FieldRestore::Raw);
        visit("pushbackLostToGsxRestart", Field(&TurnaroundData::pushbackLostToGsxRestart), FieldRestore::Raw);
        visit("jetwayOrStairsRequested", Field(&TurnaroundData::jetwayOrStairsRequested), FieldRestore::Raw);
        visit("jetwayOrStairsCompleted", Field(&TurnaroundData::jetwayOrStairsCompleted), FieldRestore::NotSaved);
        visit("stairsInPlaceTicks", Field(&TurnaroundData::stairsInPlaceTicks), FieldRestore::Restart);
        visit("gpuRequested", Field(&TurnaroundData::gpuRequested), FieldRestore::Raw);
        visit("chocksPlaced", Field(&TurnaroundData::chocksPlaced), FieldRestore::Raw);
        visit("chocksRemoved", Field(&TurnaroundData::chocksRemoved), FieldRestore::Raw);
        visit("doorsClosed", Field(&TurnaroundData::doorsClosed), FieldRestore::Raw);
        visit("ownGroundEquipmentCleared", Field(&TurnaroundData::ownGroundEquipmentCleared), FieldRestore::Raw);
        visit("arrivalGpuRequested", Field(&TurnaroundData::arrivalGpuRequested), FieldRestore::Raw);
        visit("arrivalChocksPlaced", Field(&TurnaroundData::arrivalChocksPlaced), FieldRestore::Raw);
        visit("arrivalDoorsClosed", Field(&TurnaroundData::arrivalDoorsClosed), FieldRestore::Raw);
        visit("cateringAsked", Field(&TurnaroundData::cateringAsked), FieldRestore::Raw);
        visit("cateringRequested", Field(&TurnaroundData::cateringRequested), FieldRestore::Raw);
        visit("gpuDismissRequested", Field(&TurnaroundData::gpuDismissRequested), FieldRestore::Raw);
        visit("cateringWaitIntervals", Field(&TurnaroundData::cateringWaitIntervals), FieldRestore::Raw);
        visit("lavatory.asked", Field(&TurnaroundData::lavatory, &CabinServiceProgress::asked), FieldRestore::Raw);
        visit("lavatory.requested", Field(&TurnaroundData::lavatory, &CabinServiceProgress::requested), FieldRestore::Raw);
        visit("lavatory.activeSeen", Field(&TurnaroundData::lavatory, &CabinServiceProgress::activeSeen), FieldRestore::Raw);
        visit("water.asked", Field(&TurnaroundData::water, &CabinServiceProgress::asked), FieldRestore::Raw);
        visit("water.requested", Field(&TurnaroundData::water, &CabinServiceProgress::requested), FieldRestore::Raw);
        visit("water.activeSeen", Field(&TurnaroundData::water, &CabinServiceProgress::activeSeen), FieldRestore::Raw);
        visit("cleaning.asked", Field(&TurnaroundData::cleaning, &CabinServiceProgress::asked), FieldRestore::Raw);
        visit("cleaning.requested", Field(&TurnaroundData::cleaning, &CabinServiceProgress::requested), FieldRestore::Raw);
        visit("cleaning.activeSeen", Field(&TurnaroundData::cleaning, &CabinServiceProgress::activeSeen), FieldRestore::Raw);
        visit("cabinWaitIntervals", Field(&TurnaroundData::cabinWaitIntervals), FieldRestore::Raw);
        visit("flightPlanRequested", Field(&TurnaroundData::flightPlanRequested), FieldRestore::Raw);
        visit("flightPlanRequestTicks", Field(&TurnaroundData::flightPlanRequestTicks), FieldRestore::Restart);
        visit("flightPlanRefused", Field(&TurnaroundData::flightPlanRefused), FieldRestore::Raw);
        visit("latestFlightPlanRequested", Field(&TurnaroundData::latestFlightPlanRequested), FieldRestore::Restart);
        visit("differingFlightPlanTicks", Field(&TurnaroundData::differingFlightPlanTicks), FieldRestore::Restart);
        visit("staleSimbriefGeneration", Field(&TurnaroundData::staleSimbriefGeneration), FieldRestore::NotSaved);
        visit("repositionRequested", Field(&TurnaroundData::repositionRequested), FieldRestore::Raw);
        visit("repositionCompleted", Field(&TurnaroundData::repositionCompleted), FieldRestore::Raw);
        visit("stateTickCount", Field(&TurnaroundData::stateTickCount), FieldRestore::Restart);
        visit("ruleHoldTicks", Field(&TurnaroundData::ruleHoldTicks), FieldRestore::Restart);
        visit("expiredRuleHolds", Field(&TurnaroundData::expiredRuleHolds), FieldRestore::Restart);
    }
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDDATA_H

#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_GSXGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_GSXGATEWAY_H

#include "../model/GroundPowerStatus.h"

enum class GsxStateStatus : int
{
    Unavailable = 0,
    Callable = 1,
    NotChosen = 2,
    Bypassed = 3,
    Requested = 4,
    Active = 5,
    Completed = 6,
};

enum class GsxState : int
{
    Refueling = 0,
    Boarding = 1,
    Pushback = 2,
    Deboarding = 3,
    Deice = 4,
};

enum class GroundService : int
{
    Catering,
    Lavatory,
    Water,
    Cleaning,
};

class GsxGateway
{
public:
    virtual ~GsxGateway() = default;

    [[nodiscard]] virtual bool IsAvailable() const = 0;
    [[nodiscard]] virtual GsxStateStatus GetStateStatus(GsxState gsxState) = 0;
    [[nodiscard]] virtual bool WasStateCompleted(GsxState gsxState) const = 0;
    [[nodiscard]] virtual bool IsWaitingForEngines() const = 0;
    [[nodiscard]] virtual bool IsFuelHoseConnected() const = 0;
    [[nodiscard]] virtual double GetRefuelCounterGallons() const = 0;
    [[nodiscard]] virtual bool HasPushbackStarted() const = 0;
    [[nodiscard]] virtual bool IsPushbackFinished() const = 0;
    [[nodiscard]] virtual bool IsRepositioning() const = 0;
    [[nodiscard]] virtual int GetPlannedPassengers() const = 0;
    [[nodiscard]] virtual int GetBoardedPassengers() = 0;
    [[nodiscard]] virtual int GetDeboardedPassengers() = 0;
    [[nodiscard]] virtual double GetBoardingCargoPercent() const = 0;
    [[nodiscard]] virtual double GetDeboardingCargoPercent() const = 0;
    [[nodiscard]] virtual bool AreStairsInPlace() const = 0;
    [[nodiscard]] virtual bool IsJetwayInPlace() const = 0;
    [[nodiscard]] virtual bool AreStairsAvailable() const = 0;
    [[nodiscard]] virtual bool IsJetwayAvailable() const = 0;
    [[nodiscard]] virtual bool IsSimbriefLoaded() const = 0;
    [[nodiscard]] virtual bool IsAircraftOnGround() const = 0;
    [[nodiscard]] virtual bool IsGoodEngineStartConfirmationEnabled() const = 0;
    [[nodiscard]] virtual GroundPowerStatus GetGpuStatus() const = 0;
    [[nodiscard]] virtual bool IsServiceInProgress(GroundService service) const = 0;

    virtual void TakeOverFuelAndPayload() = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_GSXGATEWAY_H

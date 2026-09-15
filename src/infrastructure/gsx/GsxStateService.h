#ifndef GSX_INTEGRATOR_CLIENT_GSXSTATESERVICE_H
#define GSX_INTEGRATOR_CLIENT_GSXSTATESERVICE_H

#include <map>
#include "../../domain/ports/GsxGateway.h"
#include "GsxRemoteState.h"

class VariableGateway;

class GsxStateService final : public GsxGateway
{
public:
    explicit GsxStateService(VariableGateway* variableGateway, const GsxRemoteState* remoteState = nullptr);

    void Reset();

    [[nodiscard]] bool IsAvailable() const;

    void Observe() override;
    [[nodiscard]] GsxStateStatus GetStateStatus(GsxState gsxState) const override;
    [[nodiscard]] bool WasStateCompleted(GsxState gsxState) const override;

    [[nodiscard]] bool IsFuelHoseConnected() const override;
    [[nodiscard]] double GetRefuelCounterGallons() const override;
    [[nodiscard]] bool HasPushbackStarted() const override;
    [[nodiscard]] bool IsPushbackFinished() const override;
    [[nodiscard]] bool IsWaitingForEngines() const override;
    [[nodiscard]] bool IsRepositioning() const override;
    [[nodiscard]] int GetPlannedPassengers() const override;
    [[nodiscard]] int GetBoardedPassengers() override;
    [[nodiscard]] int GetDeboardedPassengers() override;
    [[nodiscard]] double GetBoardingCargoPercent() override;
    [[nodiscard]] bool IsLoadingCargo() const override;
    [[nodiscard]] CargoLoader GetLoaderWaitingForDoor() const override;
    [[nodiscard]] double GetDeboardingCargoPercent() override;
    [[nodiscard]] bool AreStairsInPlace() const override;
    [[nodiscard]] bool IsJetwayInPlace() const override;
    [[nodiscard]] bool AreStairsAvailable() const override;
    [[nodiscard]] bool IsJetwayAvailable() const override;
    [[nodiscard]] bool IsJetwayOrStairsOperating() const override;
    [[nodiscard]] bool IsServiceVehicleActive() const override;
    [[nodiscard]] bool IsAircraftOnGround() const override;
    [[nodiscard]] bool IsGoodEngineStartConfirmationEnabled() const override;
    [[nodiscard]] GroundPowerStatus GetGpuStatus() const override;
    [[nodiscard]] bool IsServiceInProgress(GroundService service) const override;
    [[nodiscard]] bool OffersPushback() const override;

    void TakeOverFuelAndPayload() override;
    void ReassertTakeovers() const;

    [[nodiscard]] bool IsSimbriefLoaded() const override;
    [[nodiscard]] std::string GetSimbriefRefusal() const override;

private:
    bool fuelAndPayloadTakenOver_ = false;
    bool gpuConnectedSeenClear_ = false;
    struct StateTrack
    {
        GsxStateStatus status = GsxStateStatus::Unavailable;
        bool completed = false;
        bool couatlDiedDuringRun = false;
    };

    struct PassengerCounter
    {
        int last = 0;
        int total = 0;
        bool counting = false;
        bool moved = false;
        bool grown = false;

        int Update(int current, bool active);
    };

    struct CargoPercentReading
    {
        double first = 0.0;
        bool counting = false;
        bool moved = false;

        double Update(double current, bool active);
    };

    void ObserveState(GsxState gsxState);
    void ObserveGpuConnected();

    VariableGateway* varManager_;
    const GsxRemoteState* remote_;
    std::map<GsxState, StateTrack> states_;
    PassengerCounter boarding_;
    PassengerCounter deboarding_;
    CargoPercentReading boardingCargo_;
    CargoPercentReading deboardingCargo_;
};
#endif //GSX_INTEGRATOR_CLIENT_GSXSTATESERVICE_H

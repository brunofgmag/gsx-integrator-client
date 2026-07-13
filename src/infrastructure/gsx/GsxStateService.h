#ifndef GSX_INTEGRATOR_CLIENT_GSXSTATESERVICE_H
#define GSX_INTEGRATOR_CLIENT_GSXSTATESERVICE_H

#include <map>
#include "../../domain/ports/GsxGateway.h"

class VariableGateway;

class GsxStateService final : public GsxGateway
{
public:
    explicit GsxStateService(VariableGateway* variableGateway);

    void Reset();

    [[nodiscard]] bool IsAvailable() const override;

    [[nodiscard]] GsxStateStatus GetStateStatus(GsxState gsxState) override;
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
    [[nodiscard]] double GetBoardingCargoPercent() const override;
    [[nodiscard]] double GetDeboardingCargoPercent() const override;
    [[nodiscard]] bool AreStairsInPlace() const override;
    [[nodiscard]] bool IsJetwayInPlace() const override;
    [[nodiscard]] bool AreStairsAvailable() const override;
    [[nodiscard]] bool IsJetwayAvailable() const override;
    [[nodiscard]] bool IsAircraftOnGround() const override;
    [[nodiscard]] bool IsGoodEngineStartConfirmationEnabled() const override;
    [[nodiscard]] bool IsGpuConnected() const override;

    void TakeOverFuelAndPayload() override;

    [[nodiscard]] bool IsSimbriefLoaded() const override;

private:
    void ParseCompleted(GsxState gsxState, GsxStateStatus stateStatus);

    VariableGateway* varManager_;
    std::map<GsxState, GsxStateStatus> statesStatusMap_;
    std::map<GsxState, bool> statesCompletedMap_;

    int lastBoardingPassengers_ = 0;
    int boardingPassengersTotal_ = 0;
    int deboardingPassengersTotal_ = 0;
    int lastDeboardingPassengers_ = 0;
};
#endif //GSX_INTEGRATOR_CLIENT_GSXSTATESERVICE_H

#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSA321NEO_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSA321NEO_H

#include <memory>
#include <optional>
#include "FslabsAoc.h"
#include "../SmartSwitch.h"
#include "../../fslabs/FslabsMcduGateway.h"
#include "../../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

class FslabsA321neo final : public Aircraft
{
public:
    static constexpr auto kName = "FSLabs A321neo";

    FslabsA321neo(VariableGateway* variableGateway, const AutomationStatus* status,
                  std::unique_ptr<FslabsMcduGateway> mcdu);

    [[nodiscard]] bool IsCargoVariant() const override;

    void Observe() override;
    void OnLoadingStarted() override;
    void OnTurnaroundStarted() override;
    [[nodiscard]] bool IsReachable() const override;

    [[nodiscard]] bool LoadsItsFlightPlanThroughTheAoc() const override { return true; }
    [[nodiscard]] bool WorksThroughTheRightMcdu() const override { return true; }
    [[nodiscard]] bool IsFlightPlanLoaded() const override;
    [[nodiscard]] double GetPlannedFuelKg() const override;
    [[nodiscard]] double GetPlannedZfwKg() const override;
    [[nodiscard]] int GetPlannedPassengers() const override;
    [[nodiscard]] double GetEmptyZfwKg() const override;

    [[nodiscard]] double GetCurrentFuelKg() const override;
    [[nodiscard]] double GetCurrentZfwKg() const override;

    [[nodiscard]] bool SupportsStairsOrJetways() const override { return true; }
    [[nodiscard]] bool CompletesPushbackViaInterruptMenu() const override { return false; }
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return RefuelBy::Gsx; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return BoardBy::Self; }
    [[nodiscard]] std::optional<int> GetPassengersItBoards() const override;

    [[nodiscard]] bool RequestRefuelingItself() override;
    [[nodiscard]] bool RequestBoardingItself() override;
    [[nodiscard]] bool RequestPushbackItself() override;

    [[nodiscard]] bool ConsumeSmartSwitch() override;
    [[nodiscard]] DoorStatus GetDoorStatus() const override;
    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsHeldInPlace() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    [[nodiscard]] bool IsBeaconOn() const;
    [[nodiscard]] bool AwaitsTheInit() const;
    [[nodiscard]] bool PowerIsAsked() const;
    void CountTheShutdownTicks();

    VariableGateway* variableGateway_;
    const AutomationStatus* status_;
    std::unique_ptr<FslabsMcduGateway> mcdu_;
    FslabsAoc aoc_;
    SmartSwitch smartSwitch_;
    int shutdownTicks_ = 0;
    long long observeTicks_ = 0;
    mutable std::optional<long long> powerAskedTick_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSLABSA321NEO_H

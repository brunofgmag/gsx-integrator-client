#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11_H

#include <optional>
#include <vector>

#include "../SmartSwitch.h"
#include "../../gsx/GsxDoorSync.h"
#include "rules/TfdiMd11CargoDoorsFollowLoaderRule.h"
#include "rules/TfdiMd11CommitEfbTargetsRule.h"
#include "rules/TfdiMd11PaxDoorsFollowStairsRule.h"
#include "../../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

class TfdiMd11 final : public Aircraft
{
public:
    static constexpr auto kName = "TFDi MD-11";

    TfdiMd11(VariableGateway* variableGateway, const AutomationStatus* status, bool cargo);

    [[nodiscard]] bool IsCargoVariant() const override;

    [[nodiscard]] const std::vector<AircraftRule*>& Rules() const override;
    void Observe() override;
    void OnLoadingStarted() override {}

    [[nodiscard]] bool IsReachable() const override;
    void OnTurnaroundStarted() override;
    void OnTurnaroundResumed(const TurnaroundFacts& facts, const MemoryBag& memory) override;
    [[nodiscard]] MemoryBag TurnaroundMemory() const override;

    [[nodiscard]] bool IsFlightPlanLoaded() const override;
    [[nodiscard]] double GetPlannedFuelKg() const override;
    [[nodiscard]] double GetPlannedZfwKg() const override;
    [[nodiscard]] int GetPlannedPassengers() const override;
    [[nodiscard]] double GetEmptyZfwKg() const override;

    [[nodiscard]] double GetCurrentFuelKg() const override;
    void SetCurrentFuelKg(double fuelKg) override;
    [[nodiscard]] double GetCurrentZfwKg() const override;
    void SetCurrentZfwKg(double zfwKg) override;
    [[nodiscard]] std::optional<double> StagedFuelKg() const;
    [[nodiscard]] std::optional<double> StagedZfwKg() const;

    [[nodiscard]] bool SupportsStairsOrJetways() const override { return true; }
    [[nodiscard]] bool CompletesPushbackViaInterruptMenu() const override { return false; }
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return RefuelBy::Self; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return BoardBy::Self; }

    [[nodiscard]] DoorStatus GetDoorStatus() const override;
    void HoldDoorsClosed(bool hold) override;
    void HoldPassengerDoorsClosed(bool hold) override;
    [[nodiscard]] bool ArePassengerDoorsHeld() const;

    [[nodiscard]] bool ConsumeSmartSwitch() override;
    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] std::optional<GroundPowerStatus> GetGroundPowerStatus() const override;
    [[nodiscard]] bool SupportsChocksControl() const override { return true; }
    bool SetChocks(bool placed) override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsHeldInPlace() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    [[nodiscard]] bool IsBeaconOn() const;

    VariableGateway* variableGateway_;
    const AutomationStatus* status_;

    bool cargo_;
    SmartSwitch smartSwitch_;
    GsxDoorSync doors_;
    TfdiMd11CargoDoorsFollowLoaderRule cargoDoorRule_;
    TfdiMd11PaxDoorsFollowStairsRule paxDoorRule_;
    TfdiMd11CommitEfbTargetsRule efbTargetRule_;
    std::vector<AircraftRule*> rules_;
    std::optional<double> stagedFuelKg_;
    std::optional<double> stagedZfwKg_;
    bool heldForDeparture_ = false;
    bool passengerDoorsHeld_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11_H

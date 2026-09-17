#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJET_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJET_H

#include <optional>
#include <vector>

#include "../SmartSwitch.h"
#include "rules/FssEJetGpuFollowsRequestRule.h"
#include "rules/FssEJetKeepVendorAutomationOffRule.h"
#include "../../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

class FssEJet final : public Aircraft
{
public:
    static constexpr auto kNameE190 = "FSS Embraer E190";
    static constexpr auto kNameE195 = "FSS Embraer E195";

    FssEJet(VariableGateway* variableGateway, const AutomationStatus* status, const char* name, bool cargoVariant);

    [[nodiscard]] bool IsCargoVariant() const override;

    [[nodiscard]] const std::vector<AircraftRule*>& Rules() const override;
    void OnLoadingStarted() override {}

    [[nodiscard]] bool IsFlightPlanLoaded() const override;
    [[nodiscard]] double GetPlannedFuelKg() const override;
    [[nodiscard]] double GetPlannedZfwKg() const override;
    [[nodiscard]] int GetPlannedPassengers() const override;
    [[nodiscard]] double GetEmptyZfwKg() const override;
    [[nodiscard]] std::optional<WeightUnit> GetNativeWeightUnit() const override { return WeightUnit::Kg; }

    [[nodiscard]] double GetCurrentFuelKg() const override;
    void SetCurrentFuelKg(double) override {}
    [[nodiscard]] double GetCurrentZfwKg() const override;
    void SetCurrentZfwKg(double) override {}

    [[nodiscard]] bool SupportsStairsOrJetways() const override { return true; }
    [[nodiscard]] bool CompletesPushbackViaInterruptMenu() const override { return false; }
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return RefuelBy::Client; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return BoardBy::Client; }

    [[nodiscard]] bool ConsumeSmartSwitch() override;

    [[nodiscard]] bool SupportsGroundPowerControl() const override { return true; }
    [[nodiscard]] std::optional<GroundPowerStatus> GetGroundPowerStatus() const override;
    void SetGroundPower(bool on) override;

    [[nodiscard]] bool SupportsChocksControl() const override { return true; }
    bool SetChocks(bool placed) override;
    void ClearOwnGroundEquipment() override;

    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsHeldInPlace() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    [[nodiscard]] bool IsBeaconOn() const;
    [[nodiscard]] bool AreChocksSet() const;

    VariableGateway* variableGateway_;
    const AutomationStatus* status_;
    bool cargoVariant_;
    SmartSwitch smartSwitch_;
    FssEJetKeepVendorAutomationOffRule automationRule_;
    FssEJetGpuFollowsRequestRule gpuRule_;
    std::vector<AircraftRule*> rules_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FSSEJET_H

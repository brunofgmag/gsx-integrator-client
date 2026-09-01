#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJ_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJ_H

#include "../SmartSwitch.h"
#include "rules/AvroRjHoldForOwnAirstairRule.h"
#include "rules/AvroRjPaxDoorsServeTheAirstairRule.h"
#include "rules/AvroRjWatchModuleFuelMirrorRule.h"
#include "../../gsx/GsxDoorSync.h"
#include "../../../domain/ports/Aircraft.h"

class VariableGateway;

struct AvroRjAirstairState
{
    bool requested = false;
    bool stowed = true;
    bool settled = false;
};

struct AvroRjModuleState
{
    bool mirroringFuel = true;
};

class AvroRj final : public Aircraft
{
public:
    static constexpr auto kNameRj70 = "JustFlight Avro RJ70";
    static constexpr auto kNameRj85 = "JustFlight Avro RJ85";
    static constexpr auto kNameRj100 = "JustFlight Avro RJ100";

    AvroRj(VariableGateway* variableGateway, bool cargoVariant);

    [[nodiscard]] bool IsCargoVariant() const override;

    void Observe() override;
    void OnLoadingStarted() override;

    [[nodiscard]] bool RequiresEfbFlightPlan() const override { return true; }
    [[nodiscard]] bool IsFlightPlanLoaded() const override;
    [[nodiscard]] double GetPlannedFuelKg() const override;
    [[nodiscard]] double GetPlannedZfwKg() const override;
    [[nodiscard]] int GetPlannedPassengers() const override;
    [[nodiscard]] double GetEmptyZfwKg() const override;

    [[nodiscard]] double GetCurrentFuelKg() const override;
    void SetCurrentFuelKg(double fuelKg) override;
    [[nodiscard]] double GetCurrentZfwKg() const override;
    [[nodiscard]] double GetFuelCapacityKg() const override;
    [[nodiscard]] bool IsModuleMirroringFuel() const;

    [[nodiscard]] bool SupportsStairsOrJetways() const override;
    [[nodiscard]] bool CarriesItsOwnStairs() const override { return true; }
    [[nodiscard]] const std::vector<AircraftRule*>& Rules() const override;
    [[nodiscard]] bool IsJetwayAvailable() const;
    [[nodiscard]] bool AreAirstairsSettled() const;
    [[nodiscard]] bool IsHeldForDeparture() const;
    [[nodiscard]] bool CompletesPushbackViaInterruptMenu() const override { return false; }
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return RefuelBy::Client; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return BoardBy::Self; }

    [[nodiscard]] bool ConsumeSmartSwitch() override;

    [[nodiscard]] bool SupportsChocksControl() const override { return true; }
    bool SetChocks(bool placed) override;

    void HoldDoorsClosed(bool hold) override;

    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] DoorStatus GetDoorStatus() const override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsHeldInPlace() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    [[nodiscard]] bool IsBeaconOn() const;
    [[nodiscard]] bool AreChocksSet() const;
    [[nodiscard]] double KgPerGallon() const;

    VariableGateway* variableGateway_;
    bool cargoVariant_;
    SmartSwitch smartSwitch_;
    GsxDoorSync doors_;
    AvroRjAirstairState airstair_;
    AvroRjModuleState module_;
    bool heldForDeparture_ = false;
    double lastFuelKg_ = -1.0;
    AvroRjPaxDoorsServeTheAirstairRule doorRule_;
    AvroRjHoldForOwnAirstairRule airstairRule_;
    AvroRjWatchModuleFuelMirrorRule livenessRule_;
    std::vector<AircraftRule*> rules_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_AVRORJ_H

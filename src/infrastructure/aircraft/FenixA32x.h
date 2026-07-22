#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32X_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32X_H

#include <memory>
#include "SmartSwitch.h"
#include "../fenix/FenixEfbGateway.h"
#include "../gsx/GsxDoorSync.h"
#include "../../domain/ports/Aircraft.h"

class VariableGateway;

enum class FenixVariant { A319, A320, A321 };

class FenixA32x final : public Aircraft
{
public:
    static constexpr auto kNameA319 = "Fenix A319";
    static constexpr auto kNameA320 = "Fenix A320";
    static constexpr auto kNameA321 = "Fenix A321";

    FenixA32x(VariableGateway* variableGateway, FenixVariant variant, std::unique_ptr<FenixEfbGateway> efb);

    [[nodiscard]] const char* GetName() const override;
    [[nodiscard]] bool IsCargoVariant() const override;

    void OnTick() override;
    void OnLoadingStarted() override;
    void CloseAllDoors() override;

    [[nodiscard]] bool RequiresEfbFlightPlan() const override { return true; }
    [[nodiscard]] bool IsFlightPlanLoaded() const override;
    [[nodiscard]] double GetPlannedFuelKg() const override;
    [[nodiscard]] double GetPlannedZfwKg() const override;
    [[nodiscard]] int GetPlannedPassengers() const override;
    [[nodiscard]] double GetEmptyZfwKg() const override;
    [[nodiscard]] std::optional<WeightUnit> GetNativeWeightUnit() const override;

    [[nodiscard]] double GetCurrentFuelKg() const override;
    void SetCurrentFuelKg(double fuelKg) override;
    [[nodiscard]] double GetCurrentZfwKg() const override;
    void SetCurrentZfwKg(double zfwKg) override;

    [[nodiscard]] bool SupportsStairsOrJetways() const override { return true; }
    [[nodiscard]] bool CompletesPushbackViaInterruptMenu() const override { return false; }
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return RefuelBy::Client; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return BoardBy::Client; }

    [[nodiscard]] bool ConsumeSmartSwitch() override;
    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] std::optional<GroundPowerStatus> GetGroundPowerStatus() const override;
    [[nodiscard]] bool SupportsChocksControl() const override { return true; }
    void SetChocks(bool placed) override;
    [[nodiscard]] bool SupportsGroundPowerControl() const override { return true; }
    void SetGroundPower(bool on) override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    [[nodiscard]] bool IsBeaconOn() const;
    void EnsureEfbInitialized();
    void UpdateDoors();
    void DisarmRefuelSystemWhenDone();
    void SyncPassengersAndCargo(double zfwKg);
    void WriteSeatOccupation(int passengersOnBoard);
    void MaybeRequestFinalLoadsheet(double zfwKg);
    [[nodiscard]] double PlannedCargoKg() const;

    VariableGateway* variableGateway_;
    FenixVariant variant_;
    std::unique_ptr<FenixEfbGateway> efb_;
    GsxDoorSync doors_;
    SmartSwitch smartSwitch_;
    bool efbInitialized_ = false;
    bool finalLoadsheetRequested_ = true;
    bool refuelSystemArmed_ = false;
    double lastFuelKg_ = -1.0;
    double lastZfwKg_ = -1.0;
    int lastPassengersOnBoard_ = -1;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXA32X_H

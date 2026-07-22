#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11_H

#include "SmartSwitch.h"
#include "../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

class TfdiMd11 final : public Aircraft
{
public:
    static constexpr auto kName = "TFDi MD-11";

    TfdiMd11(VariableGateway* variableGateway, AutomationStatus* status, bool cargo);

    [[nodiscard]] const char* GetName() const override;
    [[nodiscard]] bool IsCargoVariant() const override;

    void OnTick() override;
    void OnSlowTick() override;
    void OnLoadingStarted() override {}

    [[nodiscard]] bool IsFlightPlanLoaded() const override;
    [[nodiscard]] double GetPlannedFuelKg() const override;
    [[nodiscard]] double GetPlannedZfwKg() const override;
    [[nodiscard]] int GetPlannedPassengers() const override;
    [[nodiscard]] double GetEmptyZfwKg() const override;

    [[nodiscard]] double GetCurrentFuelKg() const override;
    void SetCurrentFuelKg(double fuelKg) override;
    [[nodiscard]] double GetCurrentZfwKg() const override;
    void SetCurrentZfwKg(double zfwKg) override;

    [[nodiscard]] bool SupportsStairsOrJetways() const override { return true; }
    [[nodiscard]] bool CompletesPushbackViaInterruptMenu() const override { return false; }
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return RefuelBy::Self; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return BoardBy::Self; }

    [[nodiscard]] bool ConsumeSmartSwitch() override;
    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] std::optional<GroundPowerStatus> GetGroundPowerStatus() const override;
    [[nodiscard]] bool SupportsChocksControl() const override { return true; }
    void SetChocks(bool placed) override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    struct EfbTarget
    {
        double target = 0.0;
        double last = 0.0;
        bool seeded = false;
    };

    void UpdateTarget(EfbTarget& efbTarget, double valueKg);
    void CommitEfbTargets() const;
    void SeedTargetsIfNeeded();

    void UpdateCargoDoors();
    void UpdatePaxDoors();
    void DriveLoaderDoor(const char* loaderStateLVar, const char* doorCmdLVar, double& lastDoorTarget) const;
    void DriveStairsDoor(const char* stairsStateLVar, const char* doorCmdLVar, double& lastDoorTarget) const;

    [[nodiscard]] bool IsBeaconOn() const;

    VariableGateway* variableGateway_;
    AutomationStatus* status_;

    bool cargo_;
    SmartSwitch smartSwitch_;
    bool pendingEfbCommit_ = false;
    EfbTarget fuelTarget_;
    EfbTarget zfwTarget_;
    double fwdCargoDoorTarget_ = -1.0;
    double aftCargoDoorTarget_ = -1.0;
    double mainCargoDoorTarget_ = -1.0;
    double fwdPaxDoorTarget_ = -1.0;
    double midPaxDoorTarget_ = -1.0;
    double aftPaxDoorTarget_ = -1.0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_TFDIMD11_H

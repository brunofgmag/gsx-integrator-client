#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAX_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAX_H

#include "../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

class IFly737Max final : public Aircraft
{
public:
    IFly737Max(VariableGateway* variableGateway, AutomationStatus* status);

    [[nodiscard]] const char* GetName() const override;
    [[nodiscard]] bool IsCargoVariant() const override;

    void OnTick() override;
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
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return RefuelBy::Gsx; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return BoardBy::Client; }

    [[nodiscard]] bool ConsumeSmartSwitch() override;
    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    struct CargoDoorCloser
    {
        const char* doorName;
        const char* animLVar;
        const char* toggleLVar;
        const char* loaderLVar;
        bool unloadingSeen = false;
        bool loaderDone = false;
        int attempts = 0;
    };

    [[nodiscard]] bool IsBeaconOn() const;

    void CloseCargoDoorsAfterUnloading();
    void ArmCargoDoorCloser();
    void DisarmCargoDoorCloser();
    static void ResetDoorTracking(CargoDoorCloser& door);
    void TrackBaggageLoader(CargoDoorCloser& door) const;
    bool AdvanceDoorPulse();
    [[nodiscard]] bool IsBoardingUnderway() const;
    [[nodiscard]] bool HasPendingCargoDoorWork() const;
    [[nodiscard]] bool IsBaggageLoaderPresent(const char* loaderLVar) const;
    [[nodiscard]] bool IsDoorCloseable(const CargoDoorCloser& door) const;
    [[nodiscard]] bool IsDoorClosePending(const CargoDoorCloser& door) const;
    [[nodiscard]] CargoDoorCloser* NextCloseableDoor();

    VariableGateway* variableGateway_;
    AutomationStatus* status_;

    bool smartSwitchPressPending_ = false;
    double lastZfwKg_ = -1.0;

    CargoDoorCloser fwdCargoDoor_;
    CargoDoorCloser aftCargoDoor_;
    bool cargoDoorCloseArmed_ = false;
    CargoDoorCloser* pulseHighDoor_ = nullptr;
    int pulseSettleTicks_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAX_H

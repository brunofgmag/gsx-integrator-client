#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAX_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAX_H

#include <array>

#include "../SmartSwitch.h"
#include "../../ifly/IFlyPlanFile.h"
#include "../../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

class IFly737Max final : public Aircraft
{
public:
    IFly737Max(VariableGateway* variableGateway, const AutomationStatus* status);

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
    [[nodiscard]] double GetCurrentZfwKg() const override;
    void SetCurrentZfwKg(double zfwKg) override;

    [[nodiscard]] bool SupportsStairsOrJetways() const override { return true; }
    [[nodiscard]] bool RequiresEfbFlightPlan() const override { return true; }
    [[nodiscard]] bool CompletesPushbackViaInterruptMenu() const override { return false; }
    [[nodiscard]] RefuelBy GetRefuelMethod() const override { return RefuelBy::Gsx; }
    [[nodiscard]] BoardBy GetBoardMethod() const override { return BoardBy::Client; }

    [[nodiscard]] bool ConsumeSmartSwitch() override;
    [[nodiscard]] bool IsPowered() const override;
    [[nodiscard]] DoorStatus GetDoorStatus() const override;
    void CloseAllDoors() override;
    void HoldDoorsClosed(bool hold) override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsHeldInPlace() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

private:
    enum class CargoCycle { None, Deboarding, Boarding };

    enum class DoorKind { Cargo, JetwayOrStairs, Stairs, Catering };

    struct CargoDoorCloser
    {
        const char* doorName;
        const char* animLVar;
        const char* toggleLVar;
        const char* loaderLVar;
        DoorKind kind = DoorKind::Cargo;
        bool closeRequested = false;
        bool servedSeen = false;
        bool loaderDone = false;
        bool moving = false;
        int attempts = 0;
        int openAttempts = 0;
        int settleTicks = 0;
        int wantsOpenTicks = 0;
        bool pulseHigh = false;
        double lastAnim = -1.0;
    };

    [[nodiscard]] bool IsBeaconOn() const;

    void DriveDoors();
    void ArmCargoDoorCloser(CargoCycle cycle);
    void DisarmCargoDoorCloser();
    static void ResetDoorTracking(CargoDoorCloser& door);
    void LatchCycleCompletion();
    void TrackDoor(CargoDoorCloser& door) const;
    void TrackDoorTravel(CargoDoorCloser& door) const;
    void TrackBaggageLoader(CargoDoorCloser& door) const;
    bool AdvanceDoorPulse();
    bool AdvanceDoorPulse(CargoDoorCloser& door);
    [[nodiscard]] CargoCycle CurrentCargoCycle() const;
    [[nodiscard]] bool IsStateActive(const char* stateLVar) const;
    [[nodiscard]] bool IsStateCompleted(const char* stateLVar) const;
    [[nodiscard]] bool HasPendingCargoDoorWork() const;
    [[nodiscard]] bool IsBaggageLoaderPresent(const char* loaderLVar) const;
    [[nodiscard]] bool IsLoaderAtDoorNow(const CargoDoorCloser& door) const;
    [[nodiscard]] bool IsDoorReleased(const CargoDoorCloser& door) const;
    [[nodiscard]] bool IsDoorCloseable(const CargoDoorCloser& door) const;
    [[nodiscard]] bool IsDoorOpenable(const CargoDoorCloser& door) const;
    [[nodiscard]] bool IsDoorClosePending(const CargoDoorCloser& door) const;
    bool PulseDoor(CargoDoorCloser& door, int& attempts, const char* verb);
    [[nodiscard]] std::array<CargoDoorCloser*, 6> AllDoors();
    [[nodiscard]] bool WantsOpen(const CargoDoorCloser& door) const;

    VariableGateway* variableGateway_;
    const AutomationStatus* status_;

    SmartSwitch smartSwitch_;
    IFlyPlanImport planImport_;
    double lastZfwKg_ = -1.0;

    CargoDoorCloser fwdCargoDoor_;
    CargoDoorCloser aftCargoDoor_;
    std::array<CargoDoorCloser, 4> paxDoors_;
    bool heldForDeparture_ = false;
    CargoCycle armedCycle_ = CargoCycle::None;
    bool boardingCompleteSeen_ = false;
    bool deboardingCompleteSeen_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_IFLY737MAX_H

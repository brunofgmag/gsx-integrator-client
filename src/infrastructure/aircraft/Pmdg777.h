#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777_H

#include <array>
#include <memory>
#include <optional>
#include "SmartSwitch.h"
#include "../gsx/GsxDoorSync.h"
#include "../pmdg/Pmdg777DataGateway.h"
#include "../pmdg/Pmdg777TabletGateway.h"
#include "../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

enum class Pmdg777Variant { Er200, Lr200, Freighter, Er300 };

class Pmdg777 final : public Aircraft
{
public:
    static constexpr auto kName200Er = "PMDG 777-200ER";
    static constexpr auto kName200Lr = "PMDG 777-200LR";
    static constexpr auto kName300Er = "PMDG 777-300ER";
    static constexpr auto kNameFreighter = "PMDG 777F";

    Pmdg777(VariableGateway* variableGateway, AutomationStatus* status, Pmdg777Variant variant,
            std::unique_ptr<Pmdg777DataGateway> data, std::unique_ptr<Pmdg777TabletGateway> tablet);

    [[nodiscard]] static bool OptionsEnableDataBroadcast(const std::string& iniText);

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
    void SyncDoors();
    void SetDesiredDoor(GsxDoor door, bool open);
    void ReconcileDoors() const;
    void ReconcileGroundConn();
    void TrimZfw();
    [[nodiscard]] int DoorIndexFor(GsxDoor door) const;

    VariableGateway* variableGateway_;
    AutomationStatus* status_;
    Pmdg777Variant variant_;
    std::unique_ptr<Pmdg777DataGateway> data_;
    std::unique_ptr<Pmdg777TabletGateway> tablet_;
    GsxDoorSync doors_;
    std::array<int, 16> desiredDoor_{};
    std::array<int, static_cast<std::size_t>(GsxDoor::Count)> openedDoorIndex_{};
    std::optional<bool> desiredChocks_;
    std::optional<bool> desiredGroundPower_;
    int chocksAttempts_ = 0;
    int groundPowerAttempts_ = 0;
    int ticksSinceChocksRequest_ = 0;
    int ticksSinceGroundPowerRequest_ = 0;
    SmartSwitch smartSwitch_;
    int lastSentFuelLbs_ = -1;
    int lastSentPax_ = -1;
    int lastSentCargoLbs_ = -1;
    double lastRequestedZfwKg_ = 0.0;
    int zfwSettledTicks_ = 0;
    int zfwTrims_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777_H

#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737_H

#include <array>
#include <memory>
#include <optional>
#include "SmartSwitch.h"
#include "../gsx/GsxDoorSync.h"
#include "../pmdg/PmdgRouteFile.h"
#include "../pmdg/Pmdg737DataGateway.h"
#include "../pmdg/PmdgTabletGateway.h"
#include "../../domain/ports/Aircraft.h"

class VariableGateway;
struct AutomationStatus;

enum class Pmdg737Variant { Pax800, Bcf800, Bdsf800, Bbj2 };

class Pmdg737 final : public Aircraft
{
public:
    static constexpr auto kNamePax800 = "PMDG 737-800";
    static constexpr auto kNameBcf800 = "PMDG 737-800BCF";
    static constexpr auto kNameBdsf800 = "PMDG 737-800BDSF";
    static constexpr auto kNameBbj2 = "PMDG 737 BBJ2";

    Pmdg737(VariableGateway* variableGateway, const AutomationStatus* status, Pmdg737Variant variant,
            std::unique_ptr<Pmdg737DataGateway> data, std::unique_ptr<PmdgTabletGateway> tablet);

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
    bool SetChocks(bool placed) override;
    [[nodiscard]] bool SupportsGroundPowerControl() const override { return true; }
    void SetGroundPower(bool on) override;
    [[nodiscard]] bool IsReadyToPush() const override;
    [[nodiscard]] bool IsReadyToDeboard() const override;
    [[nodiscard]] bool IsEngineRunning() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

    [[nodiscard]] static std::optional<Pmdg737Door> DoorFor(GsxDoor door);
    [[nodiscard]] static const char* EfbDoorKey(Pmdg737Door door);

private:
    void SyncDoors();
    void SetDesiredDoor(GsxDoor door, bool open);
    void ReconcileDoors();
    [[nodiscard]] std::optional<bool> DoorIsOpen(Pmdg737Door door) const;
    void ReconcileGroundConn();
    void TrimZfw();
    [[nodiscard]] bool ChocksSet() const;

    VariableGateway* variableGateway_;
    const AutomationStatus* status_;
    Pmdg737Variant variant_;
    std::unique_ptr<Pmdg737DataGateway> data_;
    std::unique_ptr<PmdgTabletGateway> tablet_;
    GsxDoorSync doors_;
    std::array<int, static_cast<std::size_t>(Pmdg737Door::Count)> desiredDoor_{};
    std::array<int, static_cast<std::size_t>(Pmdg737Door::Count)> commandedDoor_{};
    std::array<int, static_cast<std::size_t>(Pmdg737Door::Count)> ticksSinceDoorCommand_{};
    std::array<int, static_cast<std::size_t>(Pmdg737Door::Count)> doorAttempts_{};
    std::optional<bool> desiredChocks_;
    std::optional<bool> desiredGroundPower_;
    int chocksAttempts_ = 0;
    int groundPowerAttempts_ = 0;
    int ticksSinceChocksRequest_ = 0;
    int ticksSinceGroundPowerRequest_ = 0;
    int ticksSinceStateQuery_ = 0;
    PmdgRouteImport routeImport_;
    SmartSwitch smartSwitch_;
    int lastSentFuelLbs_ = -1;
    int lastSentPax_ = -1;
    int lastSentCargoLbs_ = -1;
    double lastRequestedZfwKg_ = 0.0;
    int zfwSettledTicks_ = 0;
    int zfwTrims_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737_H

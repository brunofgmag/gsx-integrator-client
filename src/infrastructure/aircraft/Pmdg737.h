#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737_H

#include <memory>
#include <optional>
#include "PmdgAircraft.h"
#include "../pmdg/Pmdg737DataGateway.h"

enum class Pmdg737Variant { Pax800, Bcf800, Bdsf800, Bbj2 };

class Pmdg737 final : public PmdgAircraft
{
public:
    static constexpr auto kNamePax800 = "PMDG 737-800";
    static constexpr auto kNameBcf800 = "PMDG 737-800BCF";
    static constexpr auto kNameBdsf800 = "PMDG 737-800BDSF";
    static constexpr auto kNameBbj2 = "PMDG 737 BBJ2";

    Pmdg737(VariableGateway* variableGateway, const AutomationStatus* status, Pmdg737Variant variant,
            std::unique_ptr<Pmdg737DataGateway> data, std::unique_ptr<PmdgTabletGateway> tablet);

    [[nodiscard]] const char* GetName() const override;
    [[nodiscard]] bool IsParkingBrakeSet() const override;

    [[nodiscard]] static std::optional<Pmdg737Door> DoorFor(GsxDoor door);

private:
    [[nodiscard]] int DoorSlotFor(GsxDoor door) const override;
    [[nodiscard]] DoorObservation ObserveDoor(int slot) const override;
    void ToggleDoor(int slot) override;
    void RefreshDoors() override;

    [[nodiscard]] bool HasAircraftPower() const override;
    [[nodiscard]] bool GroundPowerConnected() const override;
    [[nodiscard]] bool ChocksSet() const override;

    Pmdg737Variant variant_;
    std::unique_ptr<Pmdg737DataGateway> ownedData_;
    int ticksSinceStateQuery_ = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG737_H

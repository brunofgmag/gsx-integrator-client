#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777_H

#include <memory>
#include "PmdgAircraft.h"
#include "../pmdg/Pmdg777DataGateway.h"

enum class Pmdg777Variant { Er200, Lr200, Freighter, Er300 };

class Pmdg777 final : public PmdgAircraft
{
public:
    static constexpr auto kName200Er = "PMDG 777-200ER";
    static constexpr auto kName200Lr = "PMDG 777-200LR";
    static constexpr auto kName300Er = "PMDG 777-300ER";
    static constexpr auto kNameFreighter = "PMDG 777F";

    Pmdg777(VariableGateway* variableGateway, const AutomationStatus* status, Pmdg777Variant variant,
            std::unique_ptr<Pmdg777DataGateway> data, std::unique_ptr<PmdgTabletGateway> tablet);

    [[nodiscard]] const char* GetName() const override;

private:
    [[nodiscard]] int DoorSlotFor(GsxDoor door) const override;
    [[nodiscard]] DoorObservation ObserveDoor(int slot) const override;
    void ToggleDoor(int slot) override;
    void RefreshDoors() override;

    [[nodiscard]] bool HasAircraftPower() const override;
    [[nodiscard]] bool GroundPowerConnected() const override;
    [[nodiscard]] bool GroundPowerPresent() const override;
    [[nodiscard]] bool ChocksSet() const override;

    [[nodiscard]] bool HasVendorFlightPlan() const override;

    Pmdg777Variant variant_;
    std::unique_ptr<Pmdg777DataGateway> ownedData_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDG777_H

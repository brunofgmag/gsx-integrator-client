#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_GROUNDEQUIPMENT_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_GROUNDEQUIPMENT_H

#include "../TurnaroundContext.h"
#include "../../model/GroundPowerStatus.h"

namespace turnaround
{
    inline constexpr int kGroundEquipmentGiveUpTicks = 240;

    struct GroundEquipmentWish
    {
        bool placeChocks = false;
        bool chocksCommanded = false;
        bool callGpu = false;
    };

    [[nodiscard]] GroundPowerStatus CurrentGroundPower(const TurnaroundContext& ctx);
    [[nodiscard]] bool IsGroundPowerInPlace(const TurnaroundContext& ctx, const GroundEquipmentWish& wish);
    [[nodiscard]] bool AreChocksInPlace(const TurnaroundContext& ctx, const GroundEquipmentWish& wish);
    [[nodiscard]] bool IsGroundEquipmentInPlace(const TurnaroundContext& ctx, const GroundEquipmentWish& wish);
}

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_GROUNDEQUIPMENT_H

#include "GroundEquipment.h"

#include <optional>

#include "../../model/ChocksStatus.h"
#include "../../ports/Aircraft.h"
#include "../../ports/GsxGateway.h"

namespace
{
    bool AreTheChocksWanted(const Aircraft& aircraft, const turnaround::GroundEquipmentWish& wish)
    {
        return wish.placeChocks && (aircraft.SupportsChocksControl() || aircraft.GetChocksStatus().has_value());
    }
}

namespace turnaround
{
    GroundPowerStatus CurrentGroundPower(const TurnaroundContext& ctx)
    {
        return ctx.aircraft->GetGroundPowerStatus().value_or(ctx.gsxGateway->GetGpuStatus());
    }

    bool IsGroundPowerInPlace(const TurnaroundContext& ctx, const GroundEquipmentWish& wish)
    {
        return !wish.callGpu || CurrentGroundPower(ctx) == GroundPowerStatus::Connected;
    }

    bool AreChocksInPlace(const TurnaroundContext& ctx, const GroundEquipmentWish& wish)
    {
        if (!AreTheChocksWanted(*ctx.aircraft, wish))
        {
            return true;
        }

        const std::optional<ChocksStatus> reading = ctx.aircraft->GetChocksStatus();
        if (!reading.has_value())
        {
            return wish.chocksCommanded;
        }

        return *reading == ChocksStatus::Placed;
    }

    bool IsGroundEquipmentInPlace(const TurnaroundContext& ctx, const GroundEquipmentWish& wish)
    {
        return IsGroundPowerInPlace(ctx, wish) && AreChocksInPlace(ctx, wish);
    }
}

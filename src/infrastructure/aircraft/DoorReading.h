#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DOORREADING_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DOORREADING_H

#include <optional>

#include "../simvars/VariableGateway.h"
#include "../../domain/model/DoorStatus.h"

namespace doors
{
    inline constexpr DoorStatus kNoDoorsSeen = DoorStatus::AllClosed;

    inline constexpr int kPaxDoorMovingLimitTicks = 15;
    inline constexpr int kCargoDoorMovingLimitTicks = 60;
    inline constexpr int kMainDeckDoorMovingLimitTicks = 120;

    inline DoorStatus Combine(const DoorStatus soFar, const std::optional<bool> open)
    {
        if (soFar == DoorStatus::AnyOpen || open.value_or(false))
        {
            return DoorStatus::AnyOpen;
        }

        if (!open.has_value() || soFar == DoorStatus::Unknown)
        {
            return DoorStatus::Unknown;
        }

        return DoorStatus::AllClosed;
    }

    inline std::optional<bool> OpenAboveZero(VariableReader& variables, const char* lvar)
    {
        if (!variables.HasReceivedLVar(lvar))
        {
            return std::nullopt;
        }

        return variables.GetLVar(lvar, 0.0) > 0.0;
    }
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_DOORREADING_H

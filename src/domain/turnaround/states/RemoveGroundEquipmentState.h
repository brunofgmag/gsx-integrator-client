#ifndef GSX_INTEGRATOR_CLIENT_REMOVEGROUNDEQUIPMENTSTATE_H
#define GSX_INTEGRATOR_CLIENT_REMOVEGROUNDEQUIPMENTSTATE_H

#include "TurnaroundState.h"

class RemoveGroundEquipmentState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::RemoveGroundEquipment;
    }

    [[nodiscard]] std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_REMOVEGROUNDEQUIPMENTSTATE_H

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


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;

private:
    [[nodiscard]] bool IsHeldByTheOwnGroundEquipment(TurnaroundContext& ctx);

    bool ownGroundEquipmentHoldAnnounced_ = false;
};

#endif //GSX_INTEGRATOR_CLIENT_REMOVEGROUNDEQUIPMENTSTATE_H

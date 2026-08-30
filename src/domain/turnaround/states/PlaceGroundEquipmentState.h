#ifndef GSX_INTEGRATOR_CLIENT_PLACEGROUNDEQUIPMENTSTATE_H
#define GSX_INTEGRATOR_CLIENT_PLACEGROUNDEQUIPMENTSTATE_H

#include "TurnaroundState.h"

class PlaceGroundEquipmentState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::PlaceGroundEquipment;
    }


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_PLACEGROUNDEQUIPMENTSTATE_H

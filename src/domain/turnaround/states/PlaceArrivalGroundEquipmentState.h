#ifndef GSX_INTEGRATOR_CLIENT_PLACEARRIVALGROUNDEQUIPMENTSTATE_H
#define GSX_INTEGRATOR_CLIENT_PLACEARRIVALGROUNDEQUIPMENTSTATE_H

#include "TurnaroundState.h"

class PlaceArrivalGroundEquipmentState final : public TurnaroundState
{
public:
    [[nodiscard]] TurnaroundPhase Phase() const override
    {
        return TurnaroundPhase::PlaceArrivalGroundEquipment;
    }


protected:
    [[nodiscard]] std::optional<TurnaroundTransition> EvaluatePhase(TurnaroundContext& ctx) override;
};

#endif //GSX_INTEGRATOR_CLIENT_PLACEARRIVALGROUNDEQUIPMENTSTATE_H

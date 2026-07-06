#ifndef GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATE_H
#define GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATE_H

#include <optional>
#include "../TurnaroundTransition.h"
#include "../TurnaroundPhase.h"

struct TurnaroundContext;

class TurnaroundState
{
public:
    virtual ~TurnaroundState() = default;

    [[nodiscard]] virtual TurnaroundPhase Phase() const = 0;
    [[nodiscard]] virtual std::optional<TurnaroundTransition> Evaluate(TurnaroundContext& ctx) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_DOMAIN_TURNAROUNDSTATE_H

#include "DisconnectGpuState.h"

#include "../TurnaroundContext.h"
#include "../../model/AutomationSettings.h"
#include "../../ports/GsxGateway.h"
#include "../../ports/GsxMenuGateway.h"

namespace
{
    constexpr int kRetryTicks = 60;
    constexpr int kMaxAttempts = 3;
}

std::optional<TurnaroundTransition> DisconnectGpuState::Evaluate(TurnaroundContext& ctx)
{
    const bool manageGpu = ctx.settings != nullptr && ctx.settings->callGpu == true;

    if (!manageGpu || !ctx.gsxGateway->IsGpuConnected())
    {
        return TurnaroundTransition{TurnaroundPhase::RequestPushback};
    }

    if (!ctx.data.gpuDismissRequested)
    {
        ctx.data.gpuDismissRequested = ctx.menuGateway->ToggleGpu();
        ++ctx.data.gpuDismissAttempts;

        return std::nullopt;
    }

    if (ctx.TickCondition(kRetryTicks))
    {
        if (ctx.data.gpuDismissAttempts >= kMaxAttempts)
        {
            return TurnaroundTransition{TurnaroundPhase::RequestPushback};
        }

        ctx.data.gpuDismissRequested = false;
    }

    return std::nullopt;
}

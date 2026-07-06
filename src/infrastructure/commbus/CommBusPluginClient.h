#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSPLUGINCLIENT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSPLUGINCLIENT_H

#include "../simvars/VariableGateway.h"

namespace IntegratorPluginCommBus
{
    constexpr auto kToolbarCmdLVar = "GSXI_TOOLBAR_CMD";
    constexpr auto kToolbarStateLVar = "GSXI_TOOLBAR_STATE";
    constexpr auto kGsxToolbarActiveLVar = "GSXI_GSX_TOOLBAR_ACTIVE";

    enum class ToolbarCmd : int
    {
        None = 0,
        Open = 1,
        Close = 2,
    };

    enum class ToolbarState : int
    {
        Unavailable = 0,
        Ready = 1,
        Open = 2,
    };
}

class CommBusPluginClient
{
public:
    explicit CommBusPluginClient(VariableGateway* variableGateway);

    void Setup();
    void Shutdown();

    [[nodiscard]] bool OpenGsxToolbar();
    [[nodiscard]] bool IsBridgeReady() const;
    [[nodiscard]] bool IsGsxToolbarActive() const;

private:
    [[nodiscard]] bool SendToolbarCommand(IntegratorPluginCommBus::ToolbarCmd command);

    VariableGateway* variableGateway_;
};

#endif //GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSPLUGINCLIENT_H

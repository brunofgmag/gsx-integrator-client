#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSPLUGINCLIENT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSPLUGINCLIENT_H

#include <string>
#include "CommBusBridgeGateway.h"

namespace IntegratorPluginCommBus
{
    constexpr auto kToolbarCommandChannel = "GSXI.Toolbar.Command";
    constexpr auto kToolbarStateChannel = "GSXI.Toolbar.State";
    constexpr auto kCommandOpen = "open";
    constexpr auto kCommandClose = "close";
}

class CommBusPluginClient
{
public:
    explicit CommBusPluginClient(CommBusBridgeGateway* bridge);

    void Setup();
    void Shutdown();

    [[nodiscard]] bool OpenGsxToolbar() const;
    [[nodiscard]] bool IsBridgeReady() const;
    [[nodiscard]] bool IsGsxToolbarActive() const;

private:
    void OnState(const std::string& state);

    CommBusBridgeGateway* bridge_;
    bool ready_ = false;
    bool open_ = false;
};

#endif //GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSPLUGINCLIENT_H

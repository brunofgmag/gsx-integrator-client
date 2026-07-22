#include "CommBusPluginClient.h"

#include "../logging/LogMacros.h"

using namespace IntegratorPluginCommBus;

CommBusPluginClient::CommBusPluginClient(CommBusBridgeGateway* bridge) : bridge_(bridge)
{
}

void CommBusPluginClient::Setup()
{
    bridge_->Subscribe(kToolbarStateChannel, [this](const std::string& state) { OnState(state); });
}

void CommBusPluginClient::Shutdown()
{
    bridge_->Unsubscribe(kToolbarStateChannel);
    ready_ = false;
    open_ = false;
}

void CommBusPluginClient::OnState(const std::string& state)
{
    if (state == "unavailable")
    {
        ready_ = false;
        open_ = false;

        return;
    }

    ready_ = state == "ready" || state == "open" || state == "closed";
    open_ = state == "open";
}

bool CommBusPluginClient::OpenGsxToolbar() const
{
    if (!bridge_->IsAvailable())
    {
        LOG_WARN("Integrator CommBus bridge unavailable; cannot open the GSX toolbar");

        return false;
    }

    bridge_->Call(kToolbarCommandChannel, CommBusFlag::kJs, kCommandOpen);

    return true;
}

bool CommBusPluginClient::IsBridgeReady() const
{
    return ready_;
}

bool CommBusPluginClient::IsGsxToolbarActive() const
{
    return open_;
}

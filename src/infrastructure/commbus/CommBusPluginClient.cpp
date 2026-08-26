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

bool CommBusPluginClient::SendToolbarCommand(const char* command) const
{
    if (!bridge_->IsAvailable())
    {
        LOG_WARN("Integrator CommBus bridge unavailable; cannot send '%s' to the GSX toolbar", command);

        return false;
    }

    return bridge_->Call(kToolbarCommandChannel, CommBusFlag::kJs, command);
}

bool CommBusPluginClient::OpenGsxToolbar() const
{
    return SendToolbarCommand(kCommandOpen);
}

bool CommBusPluginClient::CloseGsxToolbar() const
{
    return SendToolbarCommand(kCommandClose);
}

bool CommBusPluginClient::IsBridgeReady() const
{
    return ready_;
}

bool CommBusPluginClient::IsGsxToolbarActive() const
{
    return open_;
}

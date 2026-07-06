#include "CommBusPluginClient.h"

#include "../logging/LogMacros.h"

using namespace IntegratorPluginCommBus;

CommBusPluginClient::CommBusPluginClient(VariableGateway* variableGateway)
    : variableGateway_(variableGateway)
{
}

void CommBusPluginClient::Setup()
{
    (void)variableGateway_->GetLVar(kToolbarStateLVar, 0.0);
    (void)variableGateway_->GetLVar(kGsxToolbarActiveLVar, 0.0);
}

void CommBusPluginClient::Shutdown()
{
    variableGateway_->SetLVar(kToolbarCmdLVar, static_cast<double>(ToolbarCmd::None));
}

bool CommBusPluginClient::OpenGsxToolbar()
{
    return SendToolbarCommand(ToolbarCmd::Open);
}

bool CommBusPluginClient::IsBridgeReady() const
{
    return variableGateway_->GetLVar(kToolbarStateLVar, 0.0)
        >= static_cast<double>(ToolbarState::Ready);
}

bool CommBusPluginClient::IsGsxToolbarActive() const
{
    return variableGateway_->GetLVar(kGsxToolbarActiveLVar, 0.0) > 0.0;
}

bool CommBusPluginClient::SendToolbarCommand(const ToolbarCmd command)
{
    variableGateway_->SetLVar(kToolbarCmdLVar, static_cast<double>(command));

    if (!IsBridgeReady())
    {
        LOG_WARN("Integrator toolbar bridge not ready; queuing command %d via LVar anyway", command);

        return false;
    }

    return true;
}

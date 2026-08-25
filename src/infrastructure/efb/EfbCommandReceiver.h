#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_EFBCOMMANDRECEIVER_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_EFBCOMMANDRECEIVER_H

#include <string>
#include "../commbus/CommBusBridgeGateway.h"

class OperationsViewModel;

namespace EfbCommBus
{
    constexpr auto kCommandChannel = "GSXI.Efb.Command";
}

class EfbCommandReceiver
{
public:
    EfbCommandReceiver(CommBusBridgeGateway* bridge, OperationsViewModel* view);

    void Setup();

private:
    void Accept(const std::string& payload) const;

    CommBusBridgeGateway* bridge_;
    OperationsViewModel* view_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_EFBCOMMANDRECEIVER_H

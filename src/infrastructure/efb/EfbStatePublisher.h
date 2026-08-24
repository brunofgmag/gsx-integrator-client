#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_EFBSTATEPUBLISHER_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_EFBSTATEPUBLISHER_H

#include <functional>
#include <string>
#include "../commbus/CommBusBridgeGateway.h"
#include "../../application/sim/SimVersion.h"

class OperationsViewModel;

namespace EfbCommBus
{
    constexpr auto kStateChannel = "GSXI.Efb.State";
    constexpr auto kHelloChannel = "GSXI.Efb.Hello";
}

class EfbStatePublisher
{
public:
    EfbStatePublisher(CommBusBridgeGateway* bridge, const OperationsViewModel* view,
                      std::function<SimVersion()> simVersion);

    void Setup();
    void Publish();
    void PublishDeparture();

private:
    [[nodiscard]] bool CanPublish() const;
    void Send(std::string payload);
    void Republish();
    [[nodiscard]] std::string BuildPayload() const;

    CommBusBridgeGateway* bridge_;
    const OperationsViewModel* view_;
    std::function<SimVersion()> simVersion_;
    std::string lastPayload_;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_EFBSTATEPUBLISHER_H

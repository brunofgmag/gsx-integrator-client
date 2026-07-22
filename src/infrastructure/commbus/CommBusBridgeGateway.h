#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSBRIDGEGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSBRIDGEGATEWAY_H

#include <functional>
#include <string>

namespace CommBusFlag
{
    constexpr int kJs = 1;
    constexpr int kWasm = 2;
}

class CommBusBridgeGateway
{
public:
    using Handler = std::function<void(const std::string& payload)>;

    virtual ~CommBusBridgeGateway() = default;

    [[nodiscard]] virtual bool IsAvailable() const = 0;
    virtual void Call(const std::string& channel, int flag, const std::string& payload) = 0;
    virtual void Subscribe(const std::string& channel, int flag, Handler handler) = 0;
    void Subscribe(const std::string& channel, Handler handler)
    {
        Subscribe(channel, CommBusFlag::kWasm, std::move(handler));
    }
    virtual void Unsubscribe(const std::string& channel) = 0;

    virtual void Setup() {}
    virtual void Poll() {}
    virtual void Shutdown() {}
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSBRIDGEGATEWAY_H

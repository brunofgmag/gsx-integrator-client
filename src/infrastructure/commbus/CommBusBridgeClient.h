#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSBRIDGECLIENT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSBRIDGECLIENT_H

#include <string>
#include <unordered_map>
#include "CommBusBridgeGateway.h"
#include "../simconnect/SimConnectSession.h"

class CommBusBridgeClient final : public CommBusBridgeGateway
{
public:
    CommBusBridgeClient();
    ~CommBusBridgeClient() override;

    void Setup() override;
    void Shutdown() override;
    void Poll() override;

    [[nodiscard]] bool IsAvailable() const override;
    void Call(const std::string& channel, int flag, const std::string& payload) override;
    void Subscribe(const std::string& channel, int flag, Handler handler) override;
    using CommBusBridgeGateway::Subscribe;
    void Unsubscribe(const std::string& channel) override;

    void OnRxMessage(const std::string& json);
    void OnReadyData(const void* data, DWORD size);

    [[nodiscard]] static std::string BuildCallEnvelope(const std::string& channel,
                                                       int flag,
                                                       const std::string& payload);
    [[nodiscard]] static std::string BuildSubscribeEnvelope(const std::string& channel, int flag);
    [[nodiscard]] static std::string BuildUnsubscribeEnvelope(const std::string& channel);
    [[nodiscard]] static bool ParseInbound(const std::string& json,
                                           std::string& channelOut,
                                           std::string& payloadOut);

private:
    struct Subscription
    {
        int flag;
        Handler handler;
    };

    void SendEnvelope(const std::string& envelope) const;

    SimConnectSession session_;
    std::unordered_map<std::string, Subscription> handlers_;
    bool connected_ = false;
    bool available_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_COMMBUSBRIDGECLIENT_H

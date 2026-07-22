#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKECOMMBUSBRIDGEGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKECOMMBUSBRIDGEGATEWAY_H

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "../../src/infrastructure/commbus/CommBusBridgeGateway.h"

class FakeCommBusBridgeGateway final : public CommBusBridgeGateway
{
public:
    bool available = true;
    std::vector<std::tuple<std::string, int, std::string>> calls;
    std::vector<std::string> subscribed;
    std::vector<int> subscribedFlags;
    std::vector<std::string> unsubscribed;
    std::unordered_map<std::string, Handler> handlers;

    using CommBusBridgeGateway::Subscribe;

    [[nodiscard]] bool IsAvailable() const override { return available; }

    void Call(const std::string& channel, const int flag, const std::string& payload) override
    {
        calls.emplace_back(channel, flag, payload);
    }

    void Subscribe(const std::string& channel, const int flag, Handler handler) override
    {
        subscribed.push_back(channel);
        subscribedFlags.push_back(flag);
        handlers[channel] = std::move(handler);
    }

    void Unsubscribe(const std::string& channel) override
    {
        unsubscribed.push_back(channel);
        handlers.erase(channel);
    }

    void Deliver(const std::string& channel, const std::string& payload)
    {
        if (const auto it = handlers.find(channel); it != handlers.end() && it->second)
        {
            it->second(payload);
        }
    }

    [[nodiscard]] int CallCount(const std::string& channel) const
    {
        int count = 0;
        for (const auto& [ch, flag, payload] : calls)
        {
            if (ch == channel)
            {
                ++count;
            }
        }

        return count;
    }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKECOMMBUSBRIDGEGATEWAY_H

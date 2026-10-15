#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECT_TCP_SIMCONNECTTCPLINK_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECT_TCP_SIMCONNECTTCPLINK_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "SimConnectWire.h"

namespace simconnect_tcp
{
    class TcpLink
    {
    public:
        [[nodiscard]] static std::unique_ptr<TcpLink> Connect(const std::string& host, std::uint16_t port,
                                                              int timeoutMs);

        ~TcpLink();
        TcpLink(const TcpLink&) = delete;
        TcpLink& operator=(const TcpLink&) = delete;

        [[nodiscard]] bool Send(const std::vector<std::uint8_t>& packet);
        void Pump();
        [[nodiscard]] bool Take(Frame& frame);
        [[nodiscard]] bool IsGone() const { return closed_ || frames_.IsCorrupt(); }

    private:
        explicit TcpLink(int descriptor);

        void Flush();

        int descriptor_;
        bool closed_ = false;
        FrameBuffer frames_;
        std::vector<std::uint8_t> outgoing_;
        std::size_t sent_ = 0;
    };
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECT_TCP_SIMCONNECTTCPLINK_H

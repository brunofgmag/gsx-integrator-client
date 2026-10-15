#include "SimConnectTcpEndpoint.h"

namespace
{
    SimConnectTcpEndpoint& Endpoint()
    {
        static SimConnectTcpEndpoint endpoint;

        return endpoint;
    }

    SimConnectTcpTimeouts& Timeouts()
    {
        static SimConnectTcpTimeouts timeouts;

        return timeouts;
    }
}

void SetSimConnectTcpEndpoint(const std::string& host, const std::uint16_t port)
{
    Endpoint() = SimConnectTcpEndpoint{host, port};
}

SimConnectTcpEndpoint GetSimConnectTcpEndpoint()
{
    return Endpoint();
}

void SetSimConnectTcpTimeouts(const int connectMs, const int openReplyMs)
{
    Timeouts() = SimConnectTcpTimeouts{connectMs, openReplyMs};
}

SimConnectTcpTimeouts GetSimConnectTcpTimeouts()
{
    return Timeouts();
}

void ResetSimConnectTcpSettings()
{
    Endpoint() = SimConnectTcpEndpoint{};
    Timeouts() = SimConnectTcpTimeouts{};
}

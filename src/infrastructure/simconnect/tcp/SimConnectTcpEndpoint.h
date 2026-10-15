#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECT_TCP_SIMCONNECTTCPENDPOINT_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECT_TCP_SIMCONNECTTCPENDPOINT_H

#include <cstdint>
#include <string>

struct SimConnectTcpEndpoint
{
    std::string host = "127.0.0.1";
    std::uint16_t port = 5111;
};

struct SimConnectTcpTimeouts
{
    int connectMs = 1500;
    int openReplyMs = 5000;
};

void SetSimConnectTcpEndpoint(const std::string& host, std::uint16_t port);
[[nodiscard]] SimConnectTcpEndpoint GetSimConnectTcpEndpoint();

void SetSimConnectTcpTimeouts(int connectMs, int openReplyMs);
[[nodiscard]] SimConnectTcpTimeouts GetSimConnectTcpTimeouts();

void ResetSimConnectTcpSettings();

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECT_TCP_SIMCONNECTTCPENDPOINT_H

#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGTABLETGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGTABLETGATEWAY_H

#include <string>

class PmdgTabletGateway
{
public:
    virtual ~PmdgTabletGateway() = default;

    virtual void Poll() = 0;
    [[nodiscard]] virtual bool IsAvailable() const = 0;
    [[nodiscard]] virtual bool EfbPlanImported() const = 0;

    virtual void SendFuelTotalLbs(int lbs) = 0;
    virtual void SendPaxTotal(int count) = 0;
    virtual void SendCargoTotalLbs(int lbs) = 0;
    virtual void RequestGroundConn(const std::string& key) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGTABLETGATEWAY_H

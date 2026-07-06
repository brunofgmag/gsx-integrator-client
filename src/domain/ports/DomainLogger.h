#ifndef GSX_INTEGRATOR_CLIENT_DOMAINLOGGER_H
#define GSX_INTEGRATOR_CLIENT_DOMAINLOGGER_H

#include <string_view>

class DomainLogger
{
public:
    virtual ~DomainLogger() = default;
    virtual void LogInfo(std::string_view message) = 0;
};

#endif //GSX_INTEGRATOR_CLIENT_DOMAINLOGGER_H

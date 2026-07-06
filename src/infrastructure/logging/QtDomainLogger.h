#ifndef GSX_INTEGRATOR_CLIENT_QTDOMAINLOGGER_H
#define GSX_INTEGRATOR_CLIENT_QTDOMAINLOGGER_H

#include <string_view>
#include "../../domain/ports/DomainLogger.h"

class QtDomainLogger final : public DomainLogger
{
public:
    void LogInfo(std::string_view message) override;
};

#endif //GSX_INTEGRATOR_CLIENT_QTDOMAINLOGGER_H

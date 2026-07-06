#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEDOMAINLOGGER_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEDOMAINLOGGER_H

#include <string>
#include <vector>
#include "../../src/domain/ports/DomainLogger.h"

class FakeDomainLogger final : public DomainLogger
{
public:
    std::vector<std::string> messages;

    void LogInfo(std::string_view message) override
    {
        messages.emplace_back(message);
    }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEDOMAINLOGGER_H

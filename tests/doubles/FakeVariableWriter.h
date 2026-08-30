#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEVARIABLEWRITER_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEVARIABLEWRITER_H

#include <string>

#include "../../src/infrastructure/simvars/VariableGateway.h"

class FakeVariableWriter final : public VariableWriter
{
public:
    int setLVarCalls = 0;
    int setAVarCalls = 0;

    void SetLVar(const std::string&, double) override
    {
        ++setLVarCalls;
    }

    void SetAVar(const std::string&, const std::string&, double) override
    {
        ++setAVarCalls;
    }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEVARIABLEWRITER_H

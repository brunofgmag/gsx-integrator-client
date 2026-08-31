#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEVARIABLEWRITER_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEVARIABLEWRITER_H

#include <string>
#include <unordered_map>

#include "../../src/infrastructure/simvars/VariableGateway.h"

class FakeVariableWriter final : public VariableWriter
{
public:
    int setLVarCalls = 0;
    int setAVarCalls = 0;
    std::unordered_map<std::string, double> lvars;
    std::unordered_map<std::string, int> lvarWrites;

    void SetLVar(const std::string& name, const double value) override
    {
        ++setLVarCalls;
        ++lvarWrites[name];
        lvars[name] = value;
    }

    void SetAVar(const std::string&, const std::string&, double) override
    {
        ++setAVarCalls;
    }

    [[nodiscard]] double Written(const std::string& name, const double fallback = -1.0) const
    {
        const auto it = lvars.find(name);
        return it != lvars.end() ? it->second : fallback;
    }

    [[nodiscard]] int WriteCount(const std::string& name) const
    {
        const auto it = lvarWrites.find(name);
        return it != lvarWrites.end() ? it->second : 0;
    }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEVARIABLEWRITER_H

#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEVARIABLEGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEVARIABLEGATEWAY_H

#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../src/infrastructure/simvars/VariableGateway.h"

class FakeVariableGateway final : public VariableGateway
{
public:
    std::unordered_map<std::string, double> lvars;
    std::unordered_map<std::string, double> avars;
    std::string aircraftName;
    bool aircraftNameAvailable = true;
    std::string atcModel;
    bool atcModelAvailable = true;
    int setLVarCalls = 0;
    std::vector<std::string> fastRefreshNames;

    void SetFastRefresh(const std::string& name) override
    {
        fastRefreshNames.push_back(name);
    }

    double GetLVar(const std::string& name, const double defaultValue = 0.0) override
    {
        const auto it = lvars.find(name);
        return it != lvars.end() ? it->second : defaultValue;
    }

    bool HasReceivedLVar(const std::string& name) override
    {
        return lvars.find(name) != lvars.end();
    }

    void SetLVar(const std::string& name, const double value) override
    {
        ++setLVarCalls;
        lvars[name] = value;
    }

    double GetAVar(const std::string& name, const std::string& /*unit*/, const double defaultValue = 0.0) override
    {
        const auto it = avars.find(name);
        return it != avars.end() ? it->second : defaultValue;
    }

    bool HasReceivedAVar(const std::string& name, const std::string& /*unit*/) override
    {
        return avars.find(name) != avars.end();
    }

    bool FetchAircraftName(char* buffer, const int bufferSize) override
    {
        return CopyString(aircraftName, aircraftNameAvailable, buffer, bufferSize);
    }

    bool FetchAtcModel(char* buffer, const int bufferSize) override
    {
        return CopyString(atcModel, atcModelAvailable, buffer, bufferSize);
    }

    [[nodiscard]] double Written(const std::string& name, const double fallback = -1.0) const
    {
        const auto it = lvars.find(name);
        return it != lvars.end() ? it->second : fallback;
    }

private:
    static bool CopyString(const std::string& source, const bool available,
                           char* buffer, const int bufferSize)
    {
        if (!available || buffer == nullptr || bufferSize <= 0)
        {
            return false;
        }

        const int copyLength = std::min(
            static_cast<int>(source.size()),
            bufferSize - 1);
        std::memcpy(buffer, source.data(), static_cast<size_t>(copyLength));
        buffer[copyLength] = '\0';
        return true;
    }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEVARIABLEGATEWAY_H

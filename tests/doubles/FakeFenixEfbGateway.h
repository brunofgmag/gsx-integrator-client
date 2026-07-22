#ifndef GSX_INTEGRATOR_CLIENT_TESTS_FAKEFENIXEFBGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_TESTS_FAKEFENIXEFBGATEWAY_H

#include <string>
#include <unordered_map>
#include <vector>
#include "../../src/infrastructure/fenix/FenixEfbGateway.h"

class FakeFenixEfbGateway final : public FenixEfbGateway
{
public:
    bool available = true;
    int pollCalls = 0;
    int setFloatCalls = 0;
    int setBoolCalls = 0;
    int setStringCalls = 0;
    std::vector<std::string> subscribed;
    std::unordered_map<std::string, double> numbers;
    std::unordered_map<std::string, std::string> stringValues;
    std::unordered_map<std::string, std::vector<bool>> boolArrays;
    std::unordered_map<std::string, double> floats;
    std::unordered_map<std::string, bool> bools;
    std::unordered_map<std::string, std::string> strings;
    std::vector<std::string> loadsheetRequests;

    void Subscribe(const std::string& name) override
    {
        subscribed.push_back(name);
    }

    void Poll() override
    {
        ++pollCalls;
    }

    [[nodiscard]] bool IsAvailable() const override
    {
        return available;
    }

    [[nodiscard]] double GetNumber(const std::string& name, const double defaultValue) const override
    {
        const auto it = numbers.find(name);
        return it != numbers.end() ? it->second : defaultValue;
    }

    [[nodiscard]] std::string GetString(const std::string& name, const std::string& defaultValue) const override
    {
        const auto it = stringValues.find(name);
        return it != stringValues.end() ? it->second : defaultValue;
    }

    [[nodiscard]] std::vector<bool> GetBoolArray(const std::string& name) const override
    {
        const auto it = boolArrays.find(name);
        return it != boolArrays.end() ? it->second : std::vector<bool>{};
    }

    void SetFloat(const std::string& name, const double value) override
    {
        ++setFloatCalls;
        floats[name] = value;
    }

    void SetBool(const std::string& name, const bool value) override
    {
        ++setBoolCalls;
        bools[name] = value;
    }

    void SetString(const std::string& name, const std::string& value) override
    {
        ++setStringCalls;
        strings[name] = value;
    }

    void RequestLoadsheet(const std::string& type) override
    {
        loadsheetRequests.push_back(type);
    }

    [[nodiscard]] double WrittenFloat(const std::string& name, const double fallback = -1.0) const
    {
        const auto it = floats.find(name);
        return it != floats.end() ? it->second : fallback;
    }

    [[nodiscard]] int WrittenBool(const std::string& name, const int fallback = -1) const
    {
        const auto it = bools.find(name);
        return it != bools.end() ? (it->second ? 1 : 0) : fallback;
    }

    [[nodiscard]] std::string WrittenString(const std::string& name) const
    {
        const auto it = strings.find(name);
        return it != strings.end() ? it->second : std::string{};
    }
};

#endif // GSX_INTEGRATOR_CLIENT_TESTS_FAKEFENIXEFBGATEWAY_H

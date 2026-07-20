#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXEFBGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXEFBGATEWAY_H

#include <string>
#include <vector>

class FenixEfbGateway
{
public:
    virtual ~FenixEfbGateway() = default;

    virtual void Subscribe(const std::string& name) = 0;
    virtual void Poll() = 0;
    [[nodiscard]] virtual bool IsAvailable() const = 0;
    [[nodiscard]] virtual double GetNumber(const std::string& name, double defaultValue) const = 0;
    [[nodiscard]] virtual std::vector<bool> GetBoolArray(const std::string& name) const = 0;
    virtual void SetFloat(const std::string& name, double value) = 0;
    virtual void SetBool(const std::string& name, bool value) = 0;
    virtual void SetString(const std::string& name, const std::string& value) = 0;
    virtual void RequestLoadsheet(const std::string& type) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_FENIXEFBGATEWAY_H

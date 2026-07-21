#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_VARIABLEGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_VARIABLEGATEWAY_H

#include <string>

class VariableGateway
{
public:
    virtual ~VariableGateway() = default;

    virtual void SetFastRefresh(const std::string& name) = 0;
    virtual double GetLVar(const std::string& name, double defaultValue = 0.0) = 0;
    virtual double ConsumeLVarPeak(const std::string& name)
    {
        return GetLVar(name);
    }
    [[nodiscard]] virtual bool HasReceivedLVar(const std::string& name) = 0;
    virtual void SetLVar(const std::string& name, double value) = 0;
    virtual double GetAVar(const std::string& name, const std::string& unit, double defaultValue = 0.0) = 0;
    [[nodiscard]] virtual bool HasReceivedAVar(const std::string& name, const std::string& unit) = 0;
    virtual void SetAVar(const std::string& name, const std::string& unit, double value) = 0;
    virtual bool FetchAircraftName(char* buffer, int bufferSize) = 0;
    virtual bool FetchAtcModel(char* buffer, int bufferSize) = 0;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_VARIABLEGATEWAY_H

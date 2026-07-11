#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTVARIABLEGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTVARIABLEGATEWAY_H

#include <deque>
#include <string>
#include <unordered_map>
#include <windows.h>
#include <SimConnect.h>
#include "../simvars/VariableGateway.h"

class SimConnectVariableGateway final : public VariableGateway
{
public:
    void Attach(HANDLE hSimConnect);
    void Detach();

    void SetFastRefresh(const std::string& name) override;
    double GetLVar(const std::string& name, double defaultValue = 0.0) override;
    [[nodiscard]] bool HasReceivedLVar(const std::string& name) override;
    void SetLVar(const std::string& name, double value) override;
    double GetAVar(const std::string& name, const std::string& unit, double defaultValue = 0.0) override;
    [[nodiscard]] bool HasReceivedAVar(const std::string& name, const std::string& unit) override;
    void SetAVar(const std::string& name, const std::string& unit, double value) override;

    bool FetchAircraftName(char* buffer, int bufferSize) override;
    bool FetchAtcModel(char* buffer, int bufferSize) override;

    void HandleSimObjectData(const SIMCONNECT_RECV_SIMOBJECT_DATA* pData);

private:
    struct Slot
    {
        DWORD defineId = 0;
        std::string datumName;
        std::string unit;
        bool isString = false;
        bool registered = false;
        bool received = false;
        bool fast = false;
        double value = 0.0;
        char text[256] = {};
    };

    Slot& EnsureSlot(const std::string& key, const std::string& datumName,
                     const std::string& unit, bool isString, bool fastMode = false);
    void WriteSlot(const Slot& slot, const std::string& name, double value) const;
    bool RegisterSlot(Slot& slot) const;
    bool FetchStringSlot(const char* key, const char* datumName, char* buffer, int bufferSize);

    HANDLE hSimConnect_ = nullptr;
    std::unordered_map<std::string, std::size_t> index_;
    std::deque<Slot> slots_;
    DWORD nextDefineId_ = 1;
};

#endif //GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTVARIABLEGATEWAY_H

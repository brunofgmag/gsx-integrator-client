#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTVARIABLEGATEWAY_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTVARIABLEGATEWAY_H

#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include "SimConnectApi.h"
#include "../probe/ProbeWriteMemo.h"
#include "../simvars/VariableGateway.h"

class SimConnectVariableGateway final : public VariableGateway, public VariableTickMarker
{
public:
    void Attach(HANDLE hSimConnect);
    void Detach();
    void ForgetTextSlots();
    void SetClock(std::function<long long()> clock);

    void MarkTick() override;

    void SetFastRefresh(const std::string& name) override;
    double GetLVar(const std::string& name, double defaultValue) override;
    LVarSpan ConsumeLVarSpan(const std::string& name) override;
    [[nodiscard]] bool HasReceivedLVar(const std::string& name) override;
    [[nodiscard]] bool HasLVarChangedThisTick(const std::string& name) override;
    void SetLVar(const std::string& name, double value) override;
    double GetAVar(const std::string& name, const std::string& unit, double defaultValue) override;
    [[nodiscard]] bool HasReceivedAVar(const std::string& name, const std::string& unit) override;
    void SetAVar(const std::string& name, const std::string& unit, double value) override;

    bool FetchAircraftName(char* buffer, int bufferSize) override;
    bool FetchAtcModel(char* buffer, int bufferSize) override;

    void HandleSimObjectData(const SIMCONNECT_RECV_SIMOBJECT_DATA* pData);
    void HandleException(const SIMCONNECT_RECV_EXCEPTION* pData);
    void RetryRejected();

private:
    struct Slot
    {
        DWORD defineId = 0;
        std::string datumName;
        std::string unit;
        bool isString = false;
        bool registered = false;
        std::optional<DWORD> definitionSendId;
        std::optional<DWORD> requestSendId;
        bool retryPending = false;
        long long retryAtMs = 0;
        unsigned rejections = 0;
        bool received = false;
        bool fast = false;
        double value = 0.0;
        double spanMin = 0.0;
        double spanMax = 0.0;
        double tickValue = 0.0;
        bool tickMarked = false;
        bool changedThisTick = true;
        char text[256] = {};
    };

    Slot& EnsureSlot(const std::string& key, const std::string& datumName,
                     const std::string& unit, bool isString, bool fastMode = false);
    static void StoreSample(Slot& slot, const void* payload);
    void WriteSlot(const Slot& slot, const std::string& name, double value);
    void ReportWrite(const std::string& name, const std::string& unit, double value);
    bool RegisterSlot(Slot& slot) const;
    static void ClearRetryState(Slot& slot);
    static bool IsRetryable(const Slot& slot, DWORD exception);
    [[nodiscard]] long long NowMs() const;
    void RejectSlot(Slot& slot, DWORD exception);
    void ScheduleRetry(Slot& slot, long long nowMs);
    void PromoteToFastRefresh(Slot& slot) const;
    bool FetchStringSlot(const char* key, const char* datumName, char* buffer, int bufferSize);

    HANDLE hSimConnect_ = nullptr;
    std::unordered_map<std::string, std::size_t> index_;
    std::deque<Slot> slots_;
    DWORD nextDefineId_ = 1;
    std::function<long long()> clock_;
    probe::WriteMemo writeMemo_;
};

#endif //GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECTVARIABLEGATEWAY_H

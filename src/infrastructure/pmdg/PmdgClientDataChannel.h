#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGCLIENTDATACHANNEL_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGCLIENTDATACHANNEL_H

#include <chrono>
#include <cstring>
#include <functional>
#include <string>
#include "../logging/LogMacros.h"
#include "../simconnect/SimConnectSession.h"

struct PmdgClientDataSpec
{
    const char* connectionName;
    const char* areaName;
    SIMCONNECT_CLIENT_DATA_ID areaId;
    SIMCONNECT_CLIENT_DATA_DEFINITION_ID definitionId;
    SIMCONNECT_DATA_REQUEST_ID requestId;
    SIMCONNECT_CLIENT_DATA_PERIOD period;
    SIMCONNECT_CLIENT_DATA_REQUEST_FLAG requestFlag;
    const char* label;
};

template <typename TData>
class PmdgClientDataChannel
{
public:
    explicit PmdgClientDataChannel(const PmdgClientDataSpec& spec)
        : spec_(spec), nowMs_(&SteadyNowMs)
    {
    }

    void SetClock(std::function<long long()> clock)
    {
        nowMs_ = std::move(clock);
    }

    ~PmdgClientDataChannel()
    {
        session_.Close();
    }

    PmdgClientDataChannel(const PmdgClientDataChannel&) = delete;
    PmdgClientDataChannel& operator=(const PmdgClientDataChannel&) = delete;

    void Poll()
    {
        EnsureConnected();
        session_.Dispatch();
    }

    [[nodiscard]] bool HasData() const
    {
        return hasData_ && nowMs_() - lastFrameMs_ <= kStaleAfterMs;
    }

    [[nodiscard]] const TData& Data() const
    {
        return data_;
    }

    void SetInFlight(const bool inFlight)
    {
        inFlight_ = inFlight;
    }

    [[nodiscard]] bool InFlight() const
    {
        return inFlight_;
    }

    void TransmitEvent(const unsigned offset, const DWORD input)
    {
        session_.TransmitEvent(EventName(offset).c_str(), input);
    }

private:
    static long long SteadyNowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    static std::string EventName(const unsigned offset)
    {
        constexpr unsigned kThirdPartyEventBase = 69632;

        return "#" + std::to_string(kThirdPartyEventBase + offset);
    }

    void EnsureConnected()
    {
        if (connected_)
        {
            return;
        }

        if (!session_.Open(spec_.connectionName))
        {
            return;
        }

        connected_ = session_.RequestClientDataArea(
            spec_.areaName, spec_.areaId, spec_.definitionId, spec_.requestId,
            static_cast<DWORD>(sizeof(TData)), spec_.period, spec_.requestFlag,
            [this](const void* data, const DWORD size) { OnClientData(data, size); });

        if (!connected_)
        {
            session_.Close();
        }
    }

    void OnClientData(const void* data, const DWORD size)
    {
        if (data == nullptr || size < sizeof(TData))
        {
            return;
        }

        std::memcpy(&data_, data, sizeof(TData));
        if (data_.AircraftModel == 0 && data_.FUEL_QtyLeft <= 0.0f)
        {
            return;
        }

        if (!HasData())
        {
            LOG_INFO("%s ClientData received: model %d", spec_.label, data_.AircraftModel);
        }
        hasData_ = true;
        lastFrameMs_ = nowMs_();
    }

    static constexpr long long kStaleAfterMs = 15000;

    PmdgClientDataSpec spec_;
    SimConnectSession session_;
    std::function<long long()> nowMs_;
    TData data_{};
    long long lastFrameMs_ = 0;
    bool hasData_ = false;
    bool connected_ = false;
    bool inFlight_ = false;
};

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_PMDGCLIENTDATACHANNEL_H

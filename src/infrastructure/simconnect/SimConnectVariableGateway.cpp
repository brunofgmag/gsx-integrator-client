#include "SimConnectVariableGateway.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <optional>
#include <QtCore/QDateTime>
#include <QtCore/QString>
#include "../logging/LogMacros.h"
#include "../probe/ProbeLog.h"

namespace
{
    constexpr auto kTitleKey = "A:TITLE";
    constexpr auto kAtcModelKey = "A:ATC MODEL";
    constexpr auto kNumberUnit = "Number";
    constexpr DWORD kFastIntervalFrames = 1;
    constexpr int kShownDigits = 15;
    constexpr long long kRetryBaseMs = 1000;
    constexpr long long kRetryMaxMs = 16000;
    constexpr unsigned kRetryMaxDoublings = 4;
    constexpr unsigned kMaxRetries = 30;
    constexpr auto kLVarPrefix = "L:";
    constexpr DWORD kUnknownSendId = 0;

    long long SteadyNowMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    long long RetryDelayMs(const unsigned rejections)
    {
        const unsigned doublings = (std::min)(rejections - 1, kRetryMaxDoublings);

        return (std::min)(kRetryBaseMs << doublings, kRetryMaxMs);
    }

    void CopyTruncated(char* destination, const std::size_t capacity, const char* source)
    {
        if (capacity == 0)
        {
            return;
        }

        std::size_t length = 0;
        while (length < capacity - 1 && source[length] != '\0')
        {
            ++length;
        }

        std::memcpy(destination, source, length);
        destination[length] = '\0';
    }
}

void SimConnectVariableGateway::Attach(HANDLE hSimConnect)
{
    hSimConnect_ = hSimConnect;

    for (auto& slot : slots_)
    {
        slot.registered = false;
        ClearRetryState(slot);
        slot.received = false;
        slot.tickMarked = false;
        slot.changedThisTick = true;
        if (!RegisterSlot(slot))
        {
            LOG_WARN("Failed to re-register variable '%s'", slot.datumName.c_str());
        }
    }
}

void SimConnectVariableGateway::Detach()
{
    hSimConnect_ = nullptr;
    for (auto& slot : slots_)
    {
        slot.registered = false;
        ClearRetryState(slot);
        slot.received = false;
        slot.tickMarked = false;
        slot.changedThisTick = true;
    }
}

void SimConnectVariableGateway::ForgetTextSlots()
{
    for (auto& slot : slots_)
    {
        if (!slot.isString)
        {
            continue;
        }

        const bool wasRegistered = slot.registered;
        slot.received = false;
        slot.registered = false;
        ClearRetryState(slot);
        slot.text[0] = '\0';

        if (hSimConnect_ == nullptr)
        {
            continue;
        }

        if (wasRegistered)
        {
            (void)SimConnect_ClearDataDefinition(hSimConnect_, slot.defineId);
        }

        if (!RegisterSlot(slot))
        {
            LOG_WARN("Failed to re-register variable '%s'", slot.datumName.c_str());
        }
    }
}

void SimConnectVariableGateway::SetClock(std::function<long long()> clock)
{
    clock_ = std::move(clock);
}

long long SimConnectVariableGateway::NowMs() const
{
    return clock_ ? clock_() : SteadyNowMs();
}

void SimConnectVariableGateway::MarkTick()
{
    for (auto& slot : slots_)
    {
        if (slot.isString)
        {
            continue;
        }

        slot.changedThisTick = !slot.tickMarked || slot.value != slot.tickValue;
        slot.tickValue = slot.value;
        slot.tickMarked = true;
    }
}

void SimConnectVariableGateway::SetFastRefresh(const std::string& name)
{
    EnsureSlot("L:" + name, "L:" + name, kNumberUnit, false, true);
    if (hSimConnect_ == nullptr)
    {
        LOG_ERROR("Could not write to LVar '%s', not connected.", name.c_str());
    }
}

SimConnectVariableGateway::Slot& SimConnectVariableGateway::EnsureSlot(const std::string& key,
                                                                       const std::string& datumName,
                                                                       const std::string& unit, const bool isString,
                                                                       const bool fastMode)
{
    if (const auto it = index_.find(key); it != index_.end())
    {
        Slot& existing = slots_[it->second];
        if (fastMode && !existing.fast)
        {
            PromoteToFastRefresh(existing);
        }

        return existing;
    }

    slots_.emplace_back();
    Slot& slot = slots_.back();
    slot.defineId = nextDefineId_++;
    slot.datumName = datumName;
    slot.unit = unit;
    slot.isString = isString;
    slot.fast = fastMode;

    index_[key] = slots_.size() - 1;

    if (!RegisterSlot(slot))
    {
        LOG_WARN("Failed to register variable '%s'", datumName.c_str());
    }

    return slot;
}

void SimConnectVariableGateway::PromoteToFastRefresh(Slot& slot) const
{
    slot.fast = true;
    slot.registered = false;
    ClearRetryState(slot);

    if (!RegisterSlot(slot))
    {
        LOG_WARN("Failed to promote variable '%s' to frame rate", slot.datumName.c_str());
    }
}

bool SimConnectVariableGateway::RegisterSlot(Slot& slot) const
{
    if (slot.registered || hSimConnect_ == nullptr)
    {
        return slot.registered;
    }

    HRESULT hr;
    if (slot.isString)
    {
        hr = SimConnect_AddToDataDefinition(
            hSimConnect_, slot.defineId, slot.datumName.c_str(), nullptr,
            SIMCONNECT_DATATYPE_STRING256);
    }
    else
    {
        hr = SimConnect_AddToDataDefinition(
            hSimConnect_, slot.defineId, slot.datumName.c_str(), slot.unit.c_str(),
            SIMCONNECT_DATATYPE_FLOAT64);
    }

    if (FAILED(hr))
    {
        return false;
    }

    DWORD sendId = 0;
    if (SUCCEEDED(SimConnect_GetLastSentPacketID(hSimConnect_, &sendId)))
    {
        slot.definitionSendId = sendId;
    }

    hr = SimConnect_RequestDataOnSimObject(hSimConnect_,
                                           slot.defineId,
                                           slot.defineId,
                                           SIMCONNECT_OBJECT_ID_USER,
                                           slot.fast ? SIMCONNECT_PERIOD_SIM_FRAME : SIMCONNECT_PERIOD_SECOND,
                                           SIMCONNECT_DATA_REQUEST_FLAG_CHANGED,
                                           0,
                                           slot.fast ? kFastIntervalFrames : 0);

    if (FAILED(hr))
    {
        return false;
    }

    if (SUCCEEDED(SimConnect_GetLastSentPacketID(hSimConnect_, &sendId)))
    {
        slot.requestSendId = sendId;
    }

    slot.registered = true;
    return true;
}

void SimConnectVariableGateway::ClearRetryState(Slot& slot)
{
    slot.definitionSendId.reset();
    slot.requestSendId.reset();
    slot.retryPending = false;
    slot.retryAtMs = 0;
    slot.rejections = 0;
}

bool SimConnectVariableGateway::IsRetryable(const Slot& slot, const DWORD exception)
{
    if (exception == static_cast<DWORD>(SIMCONNECT_EXCEPTION_UNRECOGNIZED_ID))
    {
        return true;
    }

    return exception == static_cast<DWORD>(SIMCONNECT_EXCEPTION_NAME_UNRECOGNIZED)
        && slot.datumName.starts_with(kLVarPrefix);
}

void SimConnectVariableGateway::HandleException(const SIMCONNECT_RECV_EXCEPTION* pData)
{
    if (!pData || pData->dwSendID == kUnknownSendId)
    {
        return;
    }

    for (auto& slot : slots_)
    {
        if (slot.definitionSendId == pData->dwSendID || slot.requestSendId == pData->dwSendID)
        {
            RejectSlot(slot, pData->dwException);

            return;
        }
    }
}

void SimConnectVariableGateway::RejectSlot(Slot& slot, const DWORD exception)
{
    slot.registered = false;
    slot.definitionSendId.reset();
    slot.requestSendId.reset();

    if (!IsRetryable(slot, exception))
    {
        slot.retryPending = false;
        LOG_WARN("The simulator rejected variable '%s' (exception %u); it will not be requested again until reconnect",
                 slot.datumName.c_str(), static_cast<unsigned>(exception));

        return;
    }

    if (slot.rejections == 0)
    {
        LOG_INFO("The simulator rejected variable '%s' (exception %u); requesting it again",
                 slot.datumName.c_str(), static_cast<unsigned>(exception));
    }

    ScheduleRetry(slot, NowMs());
}

void SimConnectVariableGateway::ScheduleRetry(Slot& slot, const long long nowMs)
{
    ++slot.rejections;
    if (slot.rejections > kMaxRetries)
    {
        slot.retryPending = false;
        LOG_WARN("Variable '%s' is still rejected after %u attempts; giving up until reconnect",
                 slot.datumName.c_str(), kMaxRetries);

        return;
    }

    slot.retryPending = true;
    slot.retryAtMs = nowMs + RetryDelayMs(slot.rejections);
}

void SimConnectVariableGateway::RetryRejected()
{
    if (hSimConnect_ == nullptr)
    {
        return;
    }

    const long long now = NowMs();
    for (auto& slot : slots_)
    {
        if (!slot.retryPending || now < slot.retryAtMs)
        {
            continue;
        }

        slot.retryPending = false;
        (void)SimConnect_ClearDataDefinition(hSimConnect_, slot.defineId);

        if (!RegisterSlot(slot))
        {
            ScheduleRetry(slot, now);
        }
    }
}

double SimConnectVariableGateway::GetLVar(const std::string& name, const double defaultValue)
{
    const Slot& slot = EnsureSlot("L:" + name, "L:" + name, kNumberUnit, false);
    return slot.received ? slot.value : defaultValue;
}

LVarSpan SimConnectVariableGateway::ConsumeLVarSpan(const std::string& name)
{
    Slot& slot = EnsureSlot("L:" + name, "L:" + name, kNumberUnit, false);
    if (!slot.received)
    {
        return {};
    }

    const LVarSpan span{slot.spanMin, slot.spanMax, true};
    slot.spanMin = slot.value;
    slot.spanMax = slot.value;

    return span;
}

bool SimConnectVariableGateway::HasReceivedLVar(const std::string& name)
{
    return EnsureSlot("L:" + name, "L:" + name, kNumberUnit, false).received;
}

bool SimConnectVariableGateway::HasLVarChangedThisTick(const std::string& name)
{
    return EnsureSlot("L:" + name, "L:" + name, kNumberUnit, false).changedThisTick;
}

void SimConnectVariableGateway::SetLVar(const std::string& name, const double value)
{
    WriteSlot(EnsureSlot("L:" + name, "L:" + name, kNumberUnit, false), "L:" + name, value);
}

void SimConnectVariableGateway::WriteSlot(const Slot& slot, const std::string& name, const double value)
{
    if (hSimConnect_ == nullptr)
    {
        LOG_ERROR("Could not write to '%s', not connected.", name.c_str());
        return;
    }

    double payload = value;
    if (const HRESULT hr = SimConnect_SetDataOnSimObject(
            hSimConnect_, slot.defineId, SIMCONNECT_OBJECT_ID_USER, 0, 0,
            sizeof(payload), &payload);
        FAILED(hr))
    {
        LOG_ERROR("Failed to set '%s' to value %f: Err code %i",
                  name.c_str(), value, static_cast<int>(hr));

        return;
    }

    ReportWrite(name, slot.unit, value);
}

void SimConnectVariableGateway::ReportWrite(const std::string& name, const std::string& unit, const double value)
{
    if (!probe::IsOn())
    {
        return;
    }

    const QString shown = QString::number(value, 'g', kShownDigits);
    const std::optional<int> count = writeMemo_.Record(name, shown, QDateTime::currentMSecsSinceEpoch());
    if (!count)
    {
        return;
    }

    probe::Line(probe::Channel::Writes, QStringLiteral("set %1=%2 unit=%3 n=%4")
                .arg(QString::fromStdString(name), shown, QString::fromStdString(unit))
                .arg(*count));
}

double SimConnectVariableGateway::GetAVar(const std::string& name, const std::string& unit, const double defaultValue)
{
    const Slot& slot = EnsureSlot("A:" + name + "|" + unit, name, unit, false);
    return slot.received ? slot.value : defaultValue;
}

bool SimConnectVariableGateway::HasReceivedAVar(const std::string& name, const std::string& unit)
{
    return EnsureSlot("A:" + name + "|" + unit, name, unit, false).received;
}

void SimConnectVariableGateway::SetAVar(const std::string& name, const std::string& unit, const double value)
{
    WriteSlot(EnsureSlot("A:" + name + "|" + unit, name, unit, false), name, value);
}

bool SimConnectVariableGateway::FetchAircraftName(char* buffer, const int bufferSize)
{
    return FetchStringSlot(kTitleKey, "TITLE", buffer, bufferSize);
}

bool SimConnectVariableGateway::FetchAtcModel(char* buffer, const int bufferSize)
{
    return FetchStringSlot(kAtcModelKey, "ATC MODEL", buffer, bufferSize);
}

bool SimConnectVariableGateway::FetchStringSlot(const char* key, const char* datumName,
                                                char* buffer, const int bufferSize)
{
    buffer[0] = '\0';

    const Slot& slot = EnsureSlot(key, datumName, "", true);
    if (!slot.received)
    {
        return false;
    }

    CopyTruncated(buffer, static_cast<std::size_t>(bufferSize - 1), slot.text);
    buffer[bufferSize - 1] = '\0';

    return true;
}

void SimConnectVariableGateway::HandleSimObjectData(const SIMCONNECT_RECV_SIMOBJECT_DATA* pData)
{
    if (!pData)
    {
        return;
    }

    for (auto& slot : slots_)
    {
        if (slot.defineId != pData->dwRequestID)
        {
            continue;
        }

        StoreSample(slot, &pData->dwData);
        if (slot.rejections > 0)
        {
            LOG_INFO("Variable '%s' answered after %u rejections", slot.datumName.c_str(), slot.rejections);
        }
        ClearRetryState(slot);

        return;
    }
}

void SimConnectVariableGateway::StoreSample(Slot& slot, const void* payload)
{
    if (slot.isString)
    {
        CopyTruncated(slot.text, sizeof(slot.text) - 1, static_cast<const char*>(payload));
        slot.text[sizeof(slot.text) - 1] = '\0';
    }
    else
    {
        std::memcpy(&slot.value, payload, sizeof(double));
        if (!slot.received)
        {
            slot.spanMin = slot.value;
            slot.spanMax = slot.value;
        }
        else
        {
            slot.spanMin = (std::min)(slot.spanMin, slot.value);
            slot.spanMax = (std::max)(slot.spanMax, slot.value);
        }
    }

    slot.received = true;
}

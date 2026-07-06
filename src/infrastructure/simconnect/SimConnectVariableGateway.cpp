#include "SimConnectVariableGateway.h"

#include <cstring>
#include "../logging/LogMacros.h"

namespace
{
    constexpr auto kTitleKey = "A:TITLE";
    constexpr auto kAtcModelKey = "A:ATC MODEL";
    constexpr auto kNumberUnit = "Number";
    constexpr DWORD kFastIntervalFrames = 10;
}

void SimConnectVariableGateway::Attach(const HANDLE hSimConnect)
{
    hSimConnect_ = hSimConnect;

    for (auto& slot : slots_)
    {
        slot.registered = false;
        slot.received = false;
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
        slot.received = false;
    }
}

void SimConnectVariableGateway::SetFastRefresh(const std::string& name)
{
    const Slot* slot = EnsureSlot(name, name, kNumberUnit, false, true);
    if (!slot || hSimConnect_ == nullptr)
    {
        LOG_ERROR("Could not write to LVar '%s', not connected.", name.c_str());
    }
}

SimConnectVariableGateway::Slot* SimConnectVariableGateway::EnsureSlot(const std::string& key, const std::string& datumName,
                                         const std::string& unit, const bool isString, const bool fastMode)
{
    if (const auto it = index_.find(key); it != index_.end())
    {
        return &slots_[it->second];
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

    return &slot;
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

    slot.registered = true;
    return true;
}

double SimConnectVariableGateway::GetLVar(const std::string& name, const double defaultValue)
{
    const Slot* slot = EnsureSlot("L:" + name, "L:" + name, kNumberUnit, false);
    return (slot && slot->received) ? slot->value : defaultValue;
}

bool SimConnectVariableGateway::HasReceivedLVar(const std::string& name)
{
    const Slot* slot = EnsureSlot("L:" + name, "L:" + name, kNumberUnit, false);
    return slot && slot->received;
}

void SimConnectVariableGateway::SetLVar(const std::string& name, const double value)
{
    Slot* slot = EnsureSlot("L:" + name, "L:" + name, kNumberUnit, false);
    if (!slot || hSimConnect_ == nullptr)
    {
        LOG_ERROR("Could not write to LVar '%s', not connected.", name.c_str());
        return;
    }

    double payload = value;
    if (const HRESULT hr = SimConnect_SetDataOnSimObject(
            hSimConnect_, slot->defineId, SIMCONNECT_OBJECT_ID_USER, 0, 0,
            sizeof(payload), &payload);
        FAILED(hr))
    {
        LOG_ERROR("Failed to set LVar '%s' to value %f: Err code %i",
                  name.c_str(), value, static_cast<int>(hr));
    }
}

double SimConnectVariableGateway::GetAVar(const std::string& name, const std::string& unit, const double defaultValue)
{
    const Slot* slot = EnsureSlot("A:" + name + "|" + unit, name, unit, false);
    return (slot && slot->received) ? slot->value : defaultValue;
}

bool SimConnectVariableGateway::HasReceivedAVar(const std::string& name, const std::string& unit)
{
    const Slot* slot = EnsureSlot("A:" + name + "|" + unit, name, unit, false);
    return slot && slot->received;
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

    const Slot* slot = EnsureSlot(key, datumName, "", true);
    if (!slot || !slot->received)
    {
        return false;
    }

    strncpy_s(buffer, bufferSize - 1, slot->text, _TRUNCATE);
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

        if (slot.isString)
        {
            strncpy_s(
                slot.text,
                sizeof(slot.text) - 1,
                reinterpret_cast<const char*>(&pData->dwData),
                _TRUNCATE);
            slot.text[sizeof(slot.text) - 1] = '\0';
        }
        else
        {
            std::memcpy(&slot.value, &pData->dwData, sizeof(double));
        }

        slot.received = true;

        return;
    }
}

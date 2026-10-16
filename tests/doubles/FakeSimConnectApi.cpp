#include "FakeSimConnectApi.h"

SIMCONNECTAPI SimConnect_Open(HANDLE* phSimConnect, const LPCSTR szName, HWND, DWORD, HANDLE, DWORD)
{
    FakeSimConnectApi::openedNames.emplace_back(szName != nullptr ? szName : "");

    if (!FakeSimConnectApi::openSucceeds)
    {
        *phSimConnect = nullptr;

        return E_FAIL;
    }

    *phSimConnect = reinterpret_cast<HANDLE>(0x5150);
    FakeSimConnectApi::lastSentPacketId = 0;

    return S_OK;
}

SIMCONNECTAPI SimConnect_GetLastSentPacketID(HANDLE, DWORD* pdwError)
{
    if (pdwError == nullptr)
    {
        return E_INVALIDARG;
    }

    *pdwError = FakeSimConnectApi::lastSentPacketId;

    return S_OK;
}

SIMCONNECTAPI SimConnect_Close(HANDLE)
{
    return S_OK;
}

SIMCONNECTAPI SimConnect_SubscribeToSystemEvent(HANDLE, const SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                const char* SystemEventName)
{
    ++FakeSimConnectApi::lastSentPacketId;

    if (!FakeSimConnectApi::subscribeSucceeds)
    {
        return E_FAIL;
    }

    FakeSimConnectApi::systemEventSubscriptions.emplace_back(EventID, SystemEventName);

    return S_OK;
}

SIMCONNECTAPI SimConnect_MapClientEventToSimEvent(HANDLE, SIMCONNECT_CLIENT_EVENT_ID, const char* EventName)
{
    ++FakeSimConnectApi::lastSentPacketId;

    if (EventName != nullptr)
    {
        FakeSimConnectApi::mappedEventNames.emplace_back(EventName);
    }

    return FakeSimConnectApi::subscribeSucceeds ? S_OK : E_FAIL;
}

SIMCONNECTAPI SimConnect_AddClientEventToNotificationGroup(HANDLE, SIMCONNECT_NOTIFICATION_GROUP_ID,
                                                           SIMCONNECT_CLIENT_EVENT_ID, BOOL)
{
    ++FakeSimConnectApi::lastSentPacketId;

    return FakeSimConnectApi::subscribeSucceeds ? S_OK : E_FAIL;
}

SIMCONNECTAPI SimConnect_SetNotificationGroupPriority(HANDLE, SIMCONNECT_NOTIFICATION_GROUP_ID, DWORD)
{
    ++FakeSimConnectApi::lastSentPacketId;

    return FakeSimConnectApi::subscribeSucceeds ? S_OK : E_FAIL;
}

SIMCONNECTAPI SimConnect_RequestSystemState(HANDLE, SIMCONNECT_DATA_REQUEST_ID, const char* szState)
{
    ++FakeSimConnectApi::lastSentPacketId;

    FakeSimConnectApi::systemStateRequests.emplace_back(szState);

    return S_OK;
}

SIMCONNECTAPI SimConnect_TransmitClientEvent(HANDLE, SIMCONNECT_OBJECT_ID, SIMCONNECT_CLIENT_EVENT_ID,
                                             DWORD, SIMCONNECT_NOTIFICATION_GROUP_ID, SIMCONNECT_EVENT_FLAG)
{
    ++FakeSimConnectApi::lastSentPacketId;

    ++FakeSimConnectApi::transmittedEvents;

    return S_OK;
}

SIMCONNECTAPI SimConnect_CallDispatch(HANDLE, const DispatchProc pfcnDispatch, void* pContext)
{
    std::vector<std::vector<char>> messages = std::move(FakeSimConnectApi::pendingMessages);
    FakeSimConnectApi::pendingMessages.clear();

    for (auto& bytes : messages)
    {
        pfcnDispatch(reinterpret_cast<SIMCONNECT_RECV*>(bytes.data()),
                     static_cast<DWORD>(bytes.size()), pContext);
    }

    return FakeSimConnectApi::dispatchSucceeds ? S_OK : E_FAIL;
}

SIMCONNECTAPI SimConnect_AddToDataDefinition(HANDLE, const SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                             const char* DatumName, const char*, SIMCONNECT_DATATYPE, float, DWORD)
{
    ++FakeSimConnectApi::lastSentPacketId;

    if (DatumName != nullptr)
    {
        FakeSimConnectApi::dataDefinitions.emplace_back(DefineID, DatumName);
    }
    FakeSimConnectApi::definitionSendIds.emplace_back(DefineID, FakeSimConnectApi::lastSentPacketId);

    return FakeSimConnectApi::definitionsSucceed ? S_OK : E_FAIL;
}

SIMCONNECTAPI SimConnect_ClearDataDefinition(HANDLE, const SIMCONNECT_DATA_DEFINITION_ID DefineID)
{
    ++FakeSimConnectApi::lastSentPacketId;
    FakeSimConnectApi::clearedDefinitions.push_back(DefineID);

    std::erase_if(FakeSimConnectApi::dataDefinitions, [DefineID](const auto& definition)
    {
        return definition.first == DefineID;
    });

    return S_OK;
}

SIMCONNECTAPI SimConnect_RequestDataOnSimObject(HANDLE, SIMCONNECT_DATA_REQUEST_ID,
                                                const SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                                SIMCONNECT_OBJECT_ID, const SIMCONNECT_PERIOD Period,
                                                SIMCONNECT_DATA_REQUEST_FLAG, DWORD, const DWORD interval, DWORD)
{
    ++FakeSimConnectApi::lastSentPacketId;

    FakeSimConnectApi::dataRequests.push_back({DefineID, Period, interval, FakeSimConnectApi::lastSentPacketId});

    return S_OK;
}

SIMCONNECTAPI SimConnect_SetDataOnSimObject(HANDLE, const SIMCONNECT_DATA_DEFINITION_ID DefineID, SIMCONNECT_OBJECT_ID,
                                            SIMCONNECT_DATA_SET_FLAG, DWORD, const DWORD cbUnitSize,
                                            void* pDataSet)
{
    ++FakeSimConnectApi::lastSentPacketId;

    if (cbUnitSize == sizeof(double) && pDataSet != nullptr)
    {
        double value = 0.0;
        std::memcpy(&value, pDataSet, sizeof(double));
        FakeSimConnectApi::writtenSimObjectData.emplace_back(DefineID, value);
    }

    return S_OK;
}

SIMCONNECTAPI SimConnect_MapClientDataNameToID(HANDLE, const char* szClientDataName, SIMCONNECT_CLIENT_DATA_ID)
{
    ++FakeSimConnectApi::lastSentPacketId;

    if (szClientDataName != nullptr)
    {
        FakeSimConnectApi::mappedClientDataAreas.emplace_back(szClientDataName);
    }

    return S_OK;
}

SIMCONNECTAPI SimConnect_AddToClientDataDefinition(HANDLE, SIMCONNECT_CLIENT_DATA_DEFINITION_ID, DWORD, DWORD,
                                                   float, DWORD)
{
    ++FakeSimConnectApi::lastSentPacketId;

    return S_OK;
}

SIMCONNECTAPI SimConnect_RequestClientData(HANDLE, SIMCONNECT_CLIENT_DATA_ID, SIMCONNECT_DATA_REQUEST_ID,
                                           SIMCONNECT_CLIENT_DATA_DEFINITION_ID, SIMCONNECT_CLIENT_DATA_PERIOD,
                                           SIMCONNECT_CLIENT_DATA_REQUEST_FLAG, DWORD, DWORD, DWORD)
{
    ++FakeSimConnectApi::lastSentPacketId;

    return S_OK;
}

SIMCONNECTAPI SimConnect_SetClientData(HANDLE, SIMCONNECT_CLIENT_DATA_ID, SIMCONNECT_CLIENT_DATA_DEFINITION_ID,
                                       SIMCONNECT_CLIENT_DATA_SET_FLAG, DWORD, DWORD dwUnitSize, void* pDataSet)
{
    ++FakeSimConnectApi::lastSentPacketId;

    std::vector<char> bytes(dwUnitSize);
    if (dwUnitSize > 0 && pDataSet != nullptr)
    {
        std::memcpy(bytes.data(), pDataSet, dwUnitSize);
    }
    FakeSimConnectApi::writtenClientData.push_back(std::move(bytes));

    return S_OK;
}

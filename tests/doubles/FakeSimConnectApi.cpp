#include "FakeSimConnectApi.h"

SIMCONNECTAPI SimConnect_Open(HANDLE* phSimConnect, LPCSTR, HWND, DWORD, HANDLE, DWORD)
{
    if (!FakeSimConnectApi::openSucceeds)
    {
        *phSimConnect = nullptr;

        return E_FAIL;
    }

    *phSimConnect = reinterpret_cast<HANDLE>(0x5150);

    return S_OK;
}

SIMCONNECTAPI SimConnect_Close(HANDLE)
{
    return S_OK;
}

SIMCONNECTAPI SimConnect_SubscribeToSystemEvent(HANDLE, const SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                const char* SystemEventName)
{
    if (!FakeSimConnectApi::subscribeSucceeds)
    {
        return E_FAIL;
    }

    FakeSimConnectApi::systemEventSubscriptions.emplace_back(EventID, SystemEventName);

    return S_OK;
}

SIMCONNECTAPI SimConnect_MapClientEventToSimEvent(HANDLE, SIMCONNECT_CLIENT_EVENT_ID, const char*)
{
    return FakeSimConnectApi::subscribeSucceeds ? S_OK : E_FAIL;
}

SIMCONNECTAPI SimConnect_AddClientEventToNotificationGroup(HANDLE, SIMCONNECT_NOTIFICATION_GROUP_ID,
                                                           SIMCONNECT_CLIENT_EVENT_ID, BOOL)
{
    return FakeSimConnectApi::subscribeSucceeds ? S_OK : E_FAIL;
}

SIMCONNECTAPI SimConnect_SetNotificationGroupPriority(HANDLE, SIMCONNECT_NOTIFICATION_GROUP_ID, DWORD)
{
    return FakeSimConnectApi::subscribeSucceeds ? S_OK : E_FAIL;
}

SIMCONNECTAPI SimConnect_RequestSystemState(HANDLE, SIMCONNECT_DATA_REQUEST_ID, const char* szState)
{
    FakeSimConnectApi::systemStateRequests.emplace_back(szState);

    return S_OK;
}

SIMCONNECTAPI SimConnect_TransmitClientEvent(HANDLE, SIMCONNECT_OBJECT_ID, SIMCONNECT_CLIENT_EVENT_ID,
                                             DWORD, SIMCONNECT_NOTIFICATION_GROUP_ID, SIMCONNECT_EVENT_FLAG)
{
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

    return S_OK;
}

SIMCONNECTAPI SimConnect_AddToDataDefinition(HANDLE, SIMCONNECT_DATA_DEFINITION_ID, const char*, const char*,
                                             SIMCONNECT_DATATYPE, float, DWORD)
{
    return S_OK;
}

SIMCONNECTAPI SimConnect_RequestDataOnSimObject(HANDLE, SIMCONNECT_DATA_REQUEST_ID, SIMCONNECT_DATA_DEFINITION_ID,
                                                SIMCONNECT_OBJECT_ID, SIMCONNECT_PERIOD,
                                                SIMCONNECT_DATA_REQUEST_FLAG, DWORD, DWORD, DWORD)
{
    return S_OK;
}

SIMCONNECTAPI SimConnect_SetDataOnSimObject(HANDLE, SIMCONNECT_DATA_DEFINITION_ID, SIMCONNECT_OBJECT_ID,
                                            SIMCONNECT_DATA_SET_FLAG, DWORD, DWORD, void*)
{
    return S_OK;
}

#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECT_TCP_SIMCONNECTDEFINITIONS_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECT_TCP_SIMCONNECTDEFINITIONS_H

#include <cstdint>

using DWORD = std::uint32_t;
using BYTE = std::uint8_t;
using BOOL = int;
using HRESULT = std::int32_t;
using HANDLE = void*;
using HWND = void*;
using LPCSTR = const char*;

#ifndef CALLBACK
#define CALLBACK
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr) (static_cast<HRESULT>(hr) >= 0)
#endif

#ifndef FAILED
#define FAILED(hr) (static_cast<HRESULT>(hr) < 0)
#endif

inline constexpr HRESULT S_OK = 0;
inline constexpr HRESULT E_FAIL = static_cast<HRESULT>(0x80004005u);
inline constexpr HRESULT E_INVALIDARG = static_cast<HRESULT>(0x80070057u);
inline constexpr HRESULT E_HANDLE = static_cast<HRESULT>(0x80070006u);

inline constexpr DWORD MAX_PATH = 260;

#define SIMCONNECTAPI extern "C" HRESULT

using SIMCONNECT_OBJECT_ID = DWORD;
using SIMCONNECT_CLIENT_EVENT_ID = DWORD;
using SIMCONNECT_NOTIFICATION_GROUP_ID = DWORD;
using SIMCONNECT_DATA_DEFINITION_ID = DWORD;
using SIMCONNECT_DATA_REQUEST_ID = DWORD;
using SIMCONNECT_CLIENT_DATA_ID = DWORD;
using SIMCONNECT_CLIENT_DATA_DEFINITION_ID = DWORD;

using SIMCONNECT_EVENT_FLAG = DWORD;
using SIMCONNECT_DATA_REQUEST_FLAG = DWORD;
using SIMCONNECT_DATA_SET_FLAG = DWORD;
using SIMCONNECT_CLIENT_DATA_REQUEST_FLAG = DWORD;
using SIMCONNECT_CLIENT_DATA_SET_FLAG = DWORD;

inline constexpr DWORD SIMCONNECT_UNUSED = 0xFFFFFFFFu;
inline constexpr SIMCONNECT_OBJECT_ID SIMCONNECT_OBJECT_ID_USER = 0;
inline constexpr DWORD SIMCONNECT_GROUP_PRIORITY_HIGHEST = 1;

inline constexpr SIMCONNECT_EVENT_FLAG SIMCONNECT_EVENT_FLAG_DEFAULT = 0x00000000;
inline constexpr SIMCONNECT_EVENT_FLAG SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY = 0x00000010;

inline constexpr SIMCONNECT_DATA_REQUEST_FLAG SIMCONNECT_DATA_REQUEST_FLAG_DEFAULT = 0x00000000;
inline constexpr SIMCONNECT_DATA_REQUEST_FLAG SIMCONNECT_DATA_REQUEST_FLAG_CHANGED = 0x00000001;
inline constexpr SIMCONNECT_DATA_REQUEST_FLAG SIMCONNECT_DATA_REQUEST_FLAG_TAGGED = 0x00000002;

inline constexpr SIMCONNECT_DATA_SET_FLAG SIMCONNECT_DATA_SET_FLAG_DEFAULT = 0x00000000;
inline constexpr SIMCONNECT_DATA_SET_FLAG SIMCONNECT_DATA_SET_FLAG_TAGGED = 0x00000001;

inline constexpr SIMCONNECT_CLIENT_DATA_REQUEST_FLAG SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT = 0x00000000;
inline constexpr SIMCONNECT_CLIENT_DATA_REQUEST_FLAG SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED = 0x00000001;
inline constexpr SIMCONNECT_CLIENT_DATA_REQUEST_FLAG SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_TAGGED = 0x00000002;

inline constexpr SIMCONNECT_CLIENT_DATA_SET_FLAG SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT = 0x00000000;
inline constexpr SIMCONNECT_CLIENT_DATA_SET_FLAG SIMCONNECT_CLIENT_DATA_SET_FLAG_TAGGED = 0x00000001;

enum SIMCONNECT_PERIOD
{
    SIMCONNECT_PERIOD_NEVER = 0,
    SIMCONNECT_PERIOD_ONCE = 1,
    SIMCONNECT_PERIOD_VISUAL_FRAME = 2,
    SIMCONNECT_PERIOD_SIM_FRAME = 3,
    SIMCONNECT_PERIOD_SECOND = 4,
};

enum SIMCONNECT_CLIENT_DATA_PERIOD
{
    SIMCONNECT_CLIENT_DATA_PERIOD_NEVER = 0,
    SIMCONNECT_CLIENT_DATA_PERIOD_ONCE = 1,
    SIMCONNECT_CLIENT_DATA_PERIOD_VISUAL_FRAME = 2,
    SIMCONNECT_CLIENT_DATA_PERIOD_ON_SET = 3,
    SIMCONNECT_CLIENT_DATA_PERIOD_SECOND = 4,
};

enum SIMCONNECT_DATATYPE
{
    SIMCONNECT_DATATYPE_INVALID = 0,
    SIMCONNECT_DATATYPE_INT32 = 1,
    SIMCONNECT_DATATYPE_INT64 = 2,
    SIMCONNECT_DATATYPE_FLOAT32 = 3,
    SIMCONNECT_DATATYPE_FLOAT64 = 4,
    SIMCONNECT_DATATYPE_STRING8 = 5,
    SIMCONNECT_DATATYPE_STRING32 = 6,
    SIMCONNECT_DATATYPE_STRING64 = 7,
    SIMCONNECT_DATATYPE_STRING128 = 8,
    SIMCONNECT_DATATYPE_STRING256 = 9,
    SIMCONNECT_DATATYPE_STRING260 = 10,
};

enum SIMCONNECT_RECV_ID
{
    SIMCONNECT_RECV_ID_NULL = 0,
    SIMCONNECT_RECV_ID_EXCEPTION = 1,
    SIMCONNECT_RECV_ID_OPEN = 2,
    SIMCONNECT_RECV_ID_QUIT = 3,
    SIMCONNECT_RECV_ID_EVENT = 4,
    SIMCONNECT_RECV_ID_SIMOBJECT_DATA = 8,
    SIMCONNECT_RECV_ID_SYSTEM_STATE = 15,
    SIMCONNECT_RECV_ID_CLIENT_DATA = 16,
};

#pragma pack(push, 1)

struct SIMCONNECT_RECV
{
    DWORD dwSize;
    DWORD dwVersion;
    DWORD dwID;
};

struct SIMCONNECT_RECV_EXCEPTION : public SIMCONNECT_RECV
{
    DWORD dwException;
    DWORD dwSendID;
    DWORD dwIndex;
};

struct SIMCONNECT_RECV_OPEN : public SIMCONNECT_RECV
{
    char szApplicationName[256];
    DWORD dwApplicationVersionMajor;
    DWORD dwApplicationVersionMinor;
    DWORD dwApplicationBuildMajor;
    DWORD dwApplicationBuildMinor;
    DWORD dwSimConnectVersionMajor;
    DWORD dwSimConnectVersionMinor;
    DWORD dwSimConnectBuildMajor;
    DWORD dwSimConnectBuildMinor;
    DWORD dwReserved1;
    DWORD dwReserved2;
};

struct SIMCONNECT_RECV_EVENT : public SIMCONNECT_RECV
{
    DWORD uGroupID;
    DWORD uEventID;
    DWORD dwData;
};

struct SIMCONNECT_RECV_SYSTEM_STATE : public SIMCONNECT_RECV
{
    DWORD dwRequestID;
    DWORD dwInteger;
    float fFloat;
    char szString[MAX_PATH];
};

struct SIMCONNECT_RECV_SIMOBJECT_DATA : public SIMCONNECT_RECV
{
    DWORD dwRequestID;
    DWORD dwObjectID;
    DWORD dwDefineID;
    DWORD dwFlags;
    DWORD dwentrynumber;
    DWORD dwoutof;
    DWORD dwDefineCount;
    DWORD dwData;
};

struct SIMCONNECT_RECV_CLIENT_DATA : public SIMCONNECT_RECV_SIMOBJECT_DATA
{
};

#pragma pack(pop)

using DispatchProc = void (CALLBACK*)(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);

SIMCONNECTAPI SimConnect_Open(HANDLE* phSimConnect, LPCSTR szName, HWND hWnd, DWORD UserEventWin32,
                              HANDLE hEventHandle, DWORD ConfigIndex);

SIMCONNECTAPI SimConnect_Close(HANDLE hSimConnect);

SIMCONNECTAPI SimConnect_CallDispatch(HANDLE hSimConnect, DispatchProc pfcnDispatch, void* pContext);

SIMCONNECTAPI SimConnect_MapClientEventToSimEvent(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                  const char* EventName = "");

SIMCONNECTAPI SimConnect_TransmitClientEvent(HANDLE hSimConnect, SIMCONNECT_OBJECT_ID ObjectID,
                                             SIMCONNECT_CLIENT_EVENT_ID EventID, DWORD dwData,
                                             SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                             SIMCONNECT_EVENT_FLAG Flags);

SIMCONNECTAPI SimConnect_AddClientEventToNotificationGroup(HANDLE hSimConnect,
                                                           SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                                           SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                           BOOL bMaskable = 0);

SIMCONNECTAPI SimConnect_SetNotificationGroupPriority(HANDLE hSimConnect,
                                                      SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                                      DWORD uPriority);

SIMCONNECTAPI SimConnect_SubscribeToSystemEvent(HANDLE hSimConnect, SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                const char* SystemEventName);

SIMCONNECTAPI SimConnect_RequestSystemState(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                            const char* szState);

SIMCONNECTAPI SimConnect_AddToDataDefinition(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                             const char* DatumName, const char* UnitsName,
                                             SIMCONNECT_DATATYPE DatumType = SIMCONNECT_DATATYPE_FLOAT64,
                                             float fEpsilon = 0, DWORD DatumID = SIMCONNECT_UNUSED);

SIMCONNECTAPI SimConnect_ClearDataDefinition(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID DefineID);

SIMCONNECTAPI SimConnect_RequestDataOnSimObject(HANDLE hSimConnect, SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                                SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_PERIOD Period,
                                                SIMCONNECT_DATA_REQUEST_FLAG Flags = 0, DWORD origin = 0,
                                                DWORD interval = 0, DWORD limit = 0);

SIMCONNECTAPI SimConnect_SetDataOnSimObject(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                            SIMCONNECT_OBJECT_ID ObjectID, SIMCONNECT_DATA_SET_FLAG Flags,
                                            DWORD ArrayCount, DWORD cbUnitSize, void* pDataSet);

SIMCONNECTAPI SimConnect_MapClientDataNameToID(HANDLE hSimConnect, const char* szClientDataName,
                                               SIMCONNECT_CLIENT_DATA_ID ClientDataID);

SIMCONNECTAPI SimConnect_AddToClientDataDefinition(HANDLE hSimConnect,
                                                   SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                                   DWORD dwOffset, DWORD dwSizeOrType, float fEpsilon = 0,
                                                   DWORD DatumID = SIMCONNECT_UNUSED);

SIMCONNECTAPI SimConnect_RequestClientData(HANDLE hSimConnect, SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                                           SIMCONNECT_DATA_REQUEST_ID RequestID,
                                           SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                           SIMCONNECT_CLIENT_DATA_PERIOD Period = SIMCONNECT_CLIENT_DATA_PERIOD_ONCE,
                                           SIMCONNECT_CLIENT_DATA_REQUEST_FLAG Flags = 0, DWORD origin = 0,
                                           DWORD interval = 0, DWORD limit = 0);

SIMCONNECTAPI SimConnect_SetClientData(HANDLE hSimConnect, SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                                       SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                       SIMCONNECT_CLIENT_DATA_SET_FLAG Flags, DWORD dwReserved,
                                       DWORD cbUnitSize, void* pDataSet);

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECT_TCP_SIMCONNECTDEFINITIONS_H

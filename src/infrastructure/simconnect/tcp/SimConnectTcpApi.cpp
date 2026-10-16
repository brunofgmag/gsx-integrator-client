#include "../SimConnectApi.h"

#include <chrono>
#include <cstring>
#include <memory>
#include <unordered_set>
#include <utility>
#include "SimConnectTcpEndpoint.h"
#include "SimConnectTcpLink.h"
#include "SimConnectWire.h"

namespace
{
    using simconnect_tcp::Frame;
    using simconnect_tcp::PacketType;
    using simconnect_tcp::PacketWriter;
    using simconnect_tcp::TcpLink;

    constexpr std::size_t kNameLength = 256;
    constexpr std::size_t kAliasLength = 3;
    constexpr auto kAlias = "HK";
    constexpr std::uint32_t kVersionMajor = 11;
    constexpr std::uint32_t kVersionMinor = 0;
    constexpr std::uint32_t kBuildMajor = 62651;
    constexpr std::uint32_t kBuildMinor = 3;

    struct Connection
    {
        std::unique_ptr<TcpLink> link;
        std::uint32_t nextSendId = 0;
        bool openReceived = false;
        bool quitDelivered = false;
        bool closeRequested = false;
        int dispatchDepth = 0;
        std::chrono::steady_clock::time_point openDeadline;
    };

    std::unordered_set<const Connection*>& Live()
    {
        static std::unordered_set<const Connection*> live;

        return live;
    }

    Connection* Find(HANDLE handle)
    {
        const auto* connection = static_cast<const Connection*>(handle);
        if (connection == nullptr || !Live().contains(connection))
        {
            return nullptr;
        }

        return const_cast<Connection*>(connection);
    }

    HRESULT Transmit(HANDLE handle, PacketWriter& packet)
    {
        Connection* connection = Find(handle);
        if (connection == nullptr)
        {
            return E_HANDLE;
        }

        const std::uint32_t sendId = connection->nextSendId++;

        return connection->link->Send(packet.Finish(sendId)) ? S_OK : E_FAIL;
    }

    class DispatchScope
    {
    public:
        explicit DispatchScope(Connection& connection)
            : connection_(connection)
        {
            ++connection_.dispatchDepth;
        }

        DispatchScope(const DispatchScope&) = delete;
        DispatchScope& operator=(const DispatchScope&) = delete;

        ~DispatchScope()
        {
            if (--connection_.dispatchDepth == 0 && connection_.closeRequested)
            {
                delete &connection_;
            }
        }

    private:
        Connection& connection_;
    };

    void Deliver(Connection& connection, const Frame& frame, const DispatchProc proc, void* context)
    {
        const auto* header = static_cast<const SIMCONNECT_RECV*>(frame.Data());
        if (header->dwID == SIMCONNECT_RECV_ID_OPEN)
        {
            connection.openReceived = true;
        }
        else if (header->dwID == SIMCONNECT_RECV_ID_QUIT)
        {
            connection.quitDelivered = true;
        }

        proc(const_cast<SIMCONNECT_RECV*>(header), static_cast<DWORD>(frame.size), context);
    }
}

SIMCONNECTAPI SimConnect_Open(HANDLE* phSimConnect, LPCSTR szName, HWND, DWORD, HANDLE, DWORD)
{
    if (phSimConnect == nullptr)
    {
        return E_INVALIDARG;
    }

    *phSimConnect = nullptr;

    const SimConnectTcpEndpoint endpoint = GetSimConnectTcpEndpoint();
    const SimConnectTcpTimeouts timeouts = GetSimConnectTcpTimeouts();

    auto connection = std::make_unique<Connection>();
    connection->link = TcpLink::Connect(endpoint.host, endpoint.port, timeouts.connectMs);
    if (connection->link == nullptr)
    {
        return E_FAIL;
    }

    connection->openDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeouts.openReplyMs);

    PacketWriter open(PacketType::Open);
    open.Text(szName, kNameLength)
        .Uint32(0)
        .Byte(0)
        .Text(kAlias, kAliasLength)
        .Uint32(kVersionMajor)
        .Uint32(kVersionMinor)
        .Uint32(kBuildMajor)
        .Uint32(kBuildMinor);

    if (!connection->link->Send(open.Finish(connection->nextSendId++)))
    {
        return E_FAIL;
    }

    Connection* raw = connection.release();
    Live().insert(raw);
    *phSimConnect = raw;

    return S_OK;
}

SIMCONNECTAPI SimConnect_Close(HANDLE hSimConnect)
{
    Connection* connection = Find(hSimConnect);
    if (connection == nullptr)
    {
        return E_HANDLE;
    }

    Live().erase(connection);

    if (connection->dispatchDepth > 0)
    {
        connection->closeRequested = true;
    }
    else
    {
        delete connection;
    }

    return S_OK;
}

SIMCONNECTAPI SimConnect_CallDispatch(HANDLE hSimConnect, const DispatchProc pfcnDispatch, void* pContext)
{
    Connection* connection = Find(hSimConnect);
    if (connection == nullptr)
    {
        return E_HANDLE;
    }

    if (pfcnDispatch == nullptr)
    {
        return E_INVALIDARG;
    }

    const bool quitAlreadyDelivered = connection->quitDelivered;
    const DispatchScope scope(*connection);

    connection->link->Pump();

    Frame frame;
    while (!connection->closeRequested && connection->link->Take(frame))
    {
        Deliver(*connection, frame, pfcnDispatch, pContext);
    }

    if (connection->closeRequested)
    {
        return S_OK;
    }

    if (connection->link->IsGone())
    {
        if (quitAlreadyDelivered)
        {
            return E_FAIL;
        }

        if (!connection->quitDelivered)
        {
            Deliver(*connection, simconnect_tcp::MakeQuitFrame(), pfcnDispatch, pContext);
        }

        return S_OK;
    }

    if (!connection->openReceived && std::chrono::steady_clock::now() > connection->openDeadline)
    {
        return E_FAIL;
    }

    return S_OK;
}

SIMCONNECTAPI SimConnect_GetLastSentPacketID(HANDLE hSimConnect, DWORD* pdwError)
{
    if (pdwError == nullptr)
    {
        return E_INVALIDARG;
    }

    const Connection* connection = Find(hSimConnect);
    if (connection == nullptr)
    {
        return E_HANDLE;
    }

    *pdwError = connection->nextSendId - 1;

    return S_OK;
}

SIMCONNECTAPI SimConnect_MapClientEventToSimEvent(HANDLE hSimConnect, const SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                  const char* EventName)
{
    PacketWriter packet(PacketType::MapClientEventToSimEvent);
    packet.Uint32(EventID).Text(EventName, kNameLength);

    return Transmit(hSimConnect, packet);
}

SIMCONNECTAPI SimConnect_TransmitClientEvent(HANDLE hSimConnect, const SIMCONNECT_OBJECT_ID ObjectID,
                                             const SIMCONNECT_CLIENT_EVENT_ID EventID, const DWORD dwData,
                                             const SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                             const SIMCONNECT_EVENT_FLAG Flags)
{
    PacketWriter packet(PacketType::TransmitClientEvent);
    packet.Uint32(ObjectID).Uint32(EventID).Uint32(dwData).Uint32(GroupID).Uint32(Flags);

    return Transmit(hSimConnect, packet);
}

SIMCONNECTAPI SimConnect_AddClientEventToNotificationGroup(HANDLE hSimConnect,
                                                           const SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                                           const SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                           const BOOL bMaskable)
{
    PacketWriter packet(PacketType::AddClientEventToNotificationGroup);
    packet.Uint32(GroupID).Uint32(EventID).Uint32(bMaskable != 0 ? 1 : 0);

    return Transmit(hSimConnect, packet);
}

SIMCONNECTAPI SimConnect_SetNotificationGroupPriority(HANDLE hSimConnect,
                                                      const SIMCONNECT_NOTIFICATION_GROUP_ID GroupID,
                                                      const DWORD uPriority)
{
    PacketWriter packet(PacketType::SetNotificationGroupPriority);
    packet.Uint32(GroupID).Uint32(uPriority);

    return Transmit(hSimConnect, packet);
}

SIMCONNECTAPI SimConnect_SubscribeToSystemEvent(HANDLE hSimConnect, const SIMCONNECT_CLIENT_EVENT_ID EventID,
                                                const char* SystemEventName)
{
    PacketWriter packet(PacketType::SubscribeToSystemEvent);
    packet.Uint32(EventID).Text(SystemEventName, kNameLength);

    return Transmit(hSimConnect, packet);
}

SIMCONNECTAPI SimConnect_RequestSystemState(HANDLE hSimConnect, const SIMCONNECT_DATA_REQUEST_ID RequestID,
                                            const char* szState)
{
    PacketWriter packet(PacketType::RequestSystemState);
    packet.Uint32(RequestID).Text(szState, kNameLength);

    return Transmit(hSimConnect, packet);
}

SIMCONNECTAPI SimConnect_AddToDataDefinition(HANDLE hSimConnect, const SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                             const char* DatumName, const char* UnitsName,
                                             const SIMCONNECT_DATATYPE DatumType, const float fEpsilon,
                                             const DWORD DatumID)
{
    PacketWriter packet(PacketType::AddToDataDefinition);
    packet.Uint32(DefineID)
        .Text(DatumName, kNameLength)
        .Text(UnitsName, kNameLength)
        .Uint32(static_cast<std::uint32_t>(DatumType))
        .Float32(fEpsilon)
        .Uint32(DatumID);

    return Transmit(hSimConnect, packet);
}

SIMCONNECTAPI SimConnect_ClearDataDefinition(HANDLE hSimConnect, const SIMCONNECT_DATA_DEFINITION_ID DefineID)
{
    PacketWriter packet(PacketType::ClearDataDefinition);
    packet.Uint32(DefineID);

    return Transmit(hSimConnect, packet);
}

SIMCONNECTAPI SimConnect_RequestDataOnSimObject(HANDLE hSimConnect, const SIMCONNECT_DATA_REQUEST_ID RequestID,
                                                const SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                                const SIMCONNECT_OBJECT_ID ObjectID,
                                                const SIMCONNECT_PERIOD Period,
                                                const SIMCONNECT_DATA_REQUEST_FLAG Flags, const DWORD origin,
                                                const DWORD interval, const DWORD limit)
{
    PacketWriter packet(PacketType::RequestDataOnSimObject);
    packet.Uint32(RequestID)
        .Uint32(DefineID)
        .Uint32(ObjectID)
        .Uint32(static_cast<std::uint32_t>(Period))
        .Uint32(Flags)
        .Uint32(origin)
        .Uint32(interval)
        .Uint32(limit);

    return Transmit(hSimConnect, packet);
}

SIMCONNECTAPI SimConnect_SetDataOnSimObject(HANDLE hSimConnect, const SIMCONNECT_DATA_DEFINITION_ID DefineID,
                                            const SIMCONNECT_OBJECT_ID ObjectID,
                                            const SIMCONNECT_DATA_SET_FLAG Flags, const DWORD ArrayCount,
                                            const DWORD cbUnitSize, void* pDataSet)
{
    const std::uint32_t count = ArrayCount == 0 ? 1 : ArrayCount;
    const std::size_t total = static_cast<std::size_t>(cbUnitSize) * count;
    if (pDataSet == nullptr && total > 0)
    {
        return E_INVALIDARG;
    }

    PacketWriter packet(PacketType::SetDataOnSimObject);
    packet.Uint32(DefineID).Uint32(ObjectID).Uint32(Flags).Uint32(count).Uint32(cbUnitSize)
        .Bytes(pDataSet, total);

    return Transmit(hSimConnect, packet);
}

SIMCONNECTAPI SimConnect_MapClientDataNameToID(HANDLE hSimConnect, const char* szClientDataName,
                                               const SIMCONNECT_CLIENT_DATA_ID ClientDataID)
{
    PacketWriter packet(PacketType::MapClientDataNameToId);
    packet.Text(szClientDataName, kNameLength).Uint32(ClientDataID);

    return Transmit(hSimConnect, packet);
}

SIMCONNECTAPI SimConnect_AddToClientDataDefinition(HANDLE hSimConnect,
                                                   const SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                                   const DWORD dwOffset, const DWORD dwSizeOrType,
                                                   const float fEpsilon, const DWORD DatumID)
{
    PacketWriter packet(PacketType::AddToClientDataDefinition);
    packet.Uint32(DefineID).Uint32(dwOffset).Uint32(dwSizeOrType).Float32(fEpsilon).Uint32(DatumID);

    return Transmit(hSimConnect, packet);
}

SIMCONNECTAPI SimConnect_RequestClientData(HANDLE hSimConnect, const SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                                           const SIMCONNECT_DATA_REQUEST_ID RequestID,
                                           const SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                           const SIMCONNECT_CLIENT_DATA_PERIOD Period,
                                           const SIMCONNECT_CLIENT_DATA_REQUEST_FLAG Flags, const DWORD origin,
                                           const DWORD interval, const DWORD limit)
{
    PacketWriter packet(PacketType::RequestClientData);
    packet.Uint32(ClientDataID)
        .Uint32(RequestID)
        .Uint32(DefineID)
        .Uint32(static_cast<std::uint32_t>(Period))
        .Uint32(Flags)
        .Uint32(origin)
        .Uint32(interval)
        .Uint32(limit);

    return Transmit(hSimConnect, packet);
}

SIMCONNECTAPI SimConnect_SetClientData(HANDLE hSimConnect, const SIMCONNECT_CLIENT_DATA_ID ClientDataID,
                                       const SIMCONNECT_CLIENT_DATA_DEFINITION_ID DefineID,
                                       const SIMCONNECT_CLIENT_DATA_SET_FLAG Flags, const DWORD,
                                       const DWORD cbUnitSize, void* pDataSet)
{
    if (pDataSet == nullptr && cbUnitSize > 0)
    {
        return E_INVALIDARG;
    }

    PacketWriter packet(PacketType::SetClientData);
    packet.Uint32(ClientDataID).Uint32(DefineID).Uint32(Flags).Uint32(1).Uint32(cbUnitSize)
        .Bytes(pDataSet, cbUnitSize);

    return Transmit(hSimConnect, packet);
}

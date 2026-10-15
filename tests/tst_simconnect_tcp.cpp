#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <string>
#include <vector>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <QtCore/QByteArray>
#include <QtTest/QTest>

#include "../src/infrastructure/simconnect/SimConnectApi.h"
#include "../src/infrastructure/simconnect/SimConnectSession.h"
#include "../src/infrastructure/simconnect/SimConnectVariableGateway.h"
#include "../src/infrastructure/simconnect/tcp/SimConnectTcpEndpoint.h"

namespace
{
    using Bytes = std::vector<std::uint8_t>;
    using Clock = std::chrono::steady_clock;

    constexpr std::uint32_t kProtocol = 5;
    constexpr std::uint32_t kPacketTag = 0xF0000000u;
    constexpr std::size_t kHeaderSize = 16;
    constexpr std::size_t kReceiveHeaderSize = 12;
    constexpr std::size_t kNameField = 256;
    constexpr std::uint32_t kUnused = 0xFFFFFFFFu;
    constexpr int kPatient = 2000;
    constexpr int kAMoment = 30;
    constexpr std::size_t kClientDataSize = 8192;

    constexpr std::uint32_t kOpenType = 0x01;
    constexpr std::uint32_t kMapClientEventType = 0x04;
    constexpr std::uint32_t kTransmitClientEventType = 0x05;
    constexpr std::uint32_t kAddToNotificationGroupType = 0x07;
    constexpr std::uint32_t kSetGroupPriorityType = 0x09;
    constexpr std::uint32_t kAddToDataDefinitionType = 0x0c;
    constexpr std::uint32_t kClearDataDefinitionType = 0x0d;
    constexpr std::uint32_t kRequestDataOnSimObjectType = 0x0e;
    constexpr std::uint32_t kSetDataOnSimObjectType = 0x10;
    constexpr std::uint32_t kSubscribeToSystemEventType = 0x17;
    constexpr std::uint32_t kRequestSystemStateType = 0x35;
    constexpr std::uint32_t kMapClientDataNameType = 0x37;
    constexpr std::uint32_t kAddToClientDataDefinitionType = 0x39;
    constexpr std::uint32_t kRequestClientDataType = 0x3b;
    constexpr std::uint32_t kSetClientDataType = 0x3c;

    constexpr std::uint32_t kRecvException = 1;
    constexpr std::uint32_t kRecvOpen = 2;
    constexpr std::uint32_t kRecvQuit = 3;
    constexpr std::uint32_t kRecvEvent = 4;
    constexpr std::uint32_t kRecvSimObjectData = 8;
    constexpr std::uint32_t kRecvSystemState = 15;
    constexpr std::uint32_t kRecvClientData = 16;
    constexpr std::uint32_t kUnrecognizedIdException = 3;

    Bytes U32(const std::uint32_t value)
    {
        return {static_cast<std::uint8_t>(value & 0xFFu), static_cast<std::uint8_t>((value >> 8) & 0xFFu),
                static_cast<std::uint8_t>((value >> 16) & 0xFFu), static_cast<std::uint8_t>((value >> 24) & 0xFFu)};
    }

    Bytes F32(const float value)
    {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));

        return U32(bits);
    }

    Bytes F64(const double value)
    {
        Bytes bytes(sizeof(value));
        std::memcpy(bytes.data(), &value, sizeof(value));

        return bytes;
    }

    Bytes Text(const std::string& text, const std::size_t width)
    {
        Bytes bytes(width, 0);
        std::memcpy(bytes.data(), text.data(), (std::min)(text.size(), width - 1));

        return bytes;
    }

    Bytes Join(const std::initializer_list<Bytes> parts)
    {
        Bytes joined;
        for (const Bytes& part : parts)
        {
            joined.insert(joined.end(), part.begin(), part.end());
        }

        return joined;
    }

    Bytes Outgoing(const std::uint32_t type, const std::uint32_t sendId, const Bytes& payload)
    {
        return Join({U32(static_cast<std::uint32_t>(kHeaderSize + payload.size())), U32(kProtocol),
                     U32(kPacketTag | type), U32(sendId), payload});
    }

    Bytes Incoming(const std::uint32_t id, const Bytes& payload)
    {
        return Join({U32(static_cast<std::uint32_t>(kReceiveHeaderSize + payload.size())), U32(kProtocol),
                     U32(id), payload});
    }

    Bytes OpenReply()
    {
        return Incoming(kRecvOpen, Join({Text("SunRise", kNameField), U32(12), U32(2), U32(282174), U32(999),
                                       U32(12), U32(2), U32(282174), U32(999), U32(0), U32(0)}));
    }

    Bytes EventPacket(const std::uint32_t eventId, const std::uint32_t data)
    {
        return Incoming(kRecvEvent, Join({U32(kUnused), U32(eventId), U32(data)}));
    }

    Bytes SimObjectHeader(const std::uint32_t requestId)
    {
        return Join({U32(requestId), U32(0), U32(requestId), U32(0), U32(1), U32(1), U32(1)});
    }

    Bytes SimObjectDouble(const std::uint32_t requestId, const double value)
    {
        return Incoming(kRecvSimObjectData, Join({SimObjectHeader(requestId), F64(value)}));
    }

    Bytes ClientDataPacket(const std::uint32_t requestId, const Bytes& payload)
    {
        return Incoming(kRecvClientData, Join({SimObjectHeader(requestId), payload}));
    }

    QByteArray Hex(const Bytes& bytes)
    {
        return QByteArray(reinterpret_cast<const char*>(bytes.data()), static_cast<qsizetype>(bytes.size())).toHex(' ');
    }

    Bytes Pattern(const std::size_t size, const std::uint8_t seed)
    {
        Bytes bytes(size);
        for (std::size_t index = 0; index < size; ++index)
        {
            bytes[index] = static_cast<std::uint8_t>(seed + index * 31u);
        }

        return bytes;
    }

    std::uint32_t Word(const Bytes& bytes, const std::size_t offset)
    {
        std::uint32_t value = 0;
        std::memcpy(&value, bytes.data() + offset, sizeof(value));

        return value;
    }

    long ElapsedMs(const Clock::time_point since)
    {
        return static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - since).count());
    }

    class FakeSimulator
    {
    public:
        FakeSimulator()
        {
            listener_ = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);

            const int enabled = 1;
            setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

            const int smallBuffer = 4096;
            setsockopt(listener_, SOL_SOCKET, SO_RCVBUF, &smallBuffer, sizeof(smallBuffer));

            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            address.sin_port = 0;
            bind(listener_, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
            listen(listener_, 4);

            socklen_t length = sizeof(address);
            getsockname(listener_, reinterpret_cast<sockaddr*>(&address), &length);
            port_ = ntohs(address.sin_port);
        }

        FakeSimulator(const FakeSimulator&) = delete;
        FakeSimulator& operator=(const FakeSimulator&) = delete;

        ~FakeSimulator()
        {
            CloseClient();
            CloseListener();
        }

        [[nodiscard]] std::uint16_t Port() const { return port_; }

        bool Accept(const int timeoutMs = kPatient)
        {
            if (!WaitReadable(listener_, timeoutMs))
            {
                return false;
            }

            client_ = accept4(listener_, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);

            return client_ >= 0;
        }

        Bytes ReadExactly(const std::size_t count, const int timeoutMs = kPatient)
        {
            Bytes bytes;
            const Clock::time_point start = Clock::now();
            while (bytes.size() < count && ElapsedMs(start) < timeoutMs)
            {
                ReadSome(bytes, count - bytes.size(), 20);
            }

            return bytes;
        }

        Bytes ReadPacket(const int timeoutMs = kPatient)
        {
            Bytes packet = ReadExactly(sizeof(std::uint32_t), timeoutMs);
            if (packet.size() < sizeof(std::uint32_t))
            {
                return packet;
            }

            const std::uint32_t total = Word(packet, 0);
            const Bytes rest = ReadExactly(total - sizeof(std::uint32_t), timeoutMs);
            packet.insert(packet.end(), rest.begin(), rest.end());

            return packet;
        }

        std::size_t ReadSome(Bytes& into, const std::size_t most, const int timeoutMs)
        {
            if (client_ < 0 || !WaitReadable(client_, timeoutMs))
            {
                return 0;
            }

            Bytes chunk(most);
            const ssize_t received = recv(client_, chunk.data(), chunk.size(), 0);
            if (received <= 0)
            {
                return 0;
            }

            into.insert(into.end(), chunk.begin(), chunk.begin() + received);

            return static_cast<std::size_t>(received);
        }

        void Send(const Bytes& bytes)
        {
            std::size_t sent = 0;
            const Clock::time_point start = Clock::now();
            while (sent < bytes.size() && ElapsedMs(start) < kPatient)
            {
                const ssize_t written = send(client_, bytes.data() + sent, bytes.size() - sent, MSG_NOSIGNAL);
                if (written > 0)
                {
                    sent += static_cast<std::size_t>(written);
                    continue;
                }

                pollfd pending{};
                pending.fd = client_;
                pending.events = POLLOUT;
                poll(&pending, 1, 20);
            }
        }

        void CloseClient()
        {
            if (client_ >= 0)
            {
                close(client_);
                client_ = -1;
            }
        }

        void CloseListener()
        {
            if (listener_ >= 0)
            {
                close(listener_);
                listener_ = -1;
            }
        }

    private:
        static bool WaitReadable(const int descriptor, const int timeoutMs)
        {
            pollfd pending{};
            pending.fd = descriptor;
            pending.events = POLLIN;

            return poll(&pending, 1, timeoutMs) > 0;
        }

        int listener_ = -1;
        int client_ = -1;
        std::uint16_t port_ = 0;
    };

    struct Message
    {
        std::uint32_t id = 0;
        std::uint32_t declaredSize = 0;
        Bytes bytes;
    };

    struct Recorder
    {
        std::vector<Message> messages;
        std::function<void()> onMessage;

        static void CALLBACK Collect(SIMCONNECT_RECV* data, const DWORD size, void* context)
        {
            auto* self = static_cast<Recorder*>(context);
            Message message;
            message.id = data->dwID;
            message.declaredSize = size;
            const auto* raw = reinterpret_cast<const std::uint8_t*>(data);
            message.bytes.assign(raw, raw + size);
            self->messages.push_back(std::move(message));
            if (self->onMessage)
            {
                self->onMessage();
            }
        }
    };

    struct Rig
    {
        FakeSimulator simulator;
        HANDLE handle = nullptr;
        Bytes openPacket;

        Rig()
        {
            SetSimConnectTcpEndpoint("127.0.0.1", simulator.Port());
        }

        Rig(const Rig&) = delete;
        Rig& operator=(const Rig&) = delete;

        ~Rig()
        {
            if (handle != nullptr)
            {
                SimConnect_Close(handle);
            }
        }

        bool Connect()
        {
            if (FAILED(SimConnect_Open(&handle, "GSX Integrator", nullptr, 0, nullptr, 0)))
            {
                return false;
            }

            if (!simulator.Accept())
            {
                return false;
            }

            openPacket = simulator.ReadPacket();

            return openPacket.size() > kHeaderSize;
        }

        HRESULT Dispatch(Recorder& recorder) const
        {
            return SimConnect_CallDispatch(handle, &Recorder::Collect, &recorder);
        }

        bool DispatchUntil(Recorder& recorder, const std::size_t count, const int timeoutMs = kPatient) const
        {
            const Clock::time_point start = Clock::now();
            while (recorder.messages.size() < count && ElapsedMs(start) < timeoutMs)
            {
                if (FAILED(Dispatch(recorder)))
                {
                    return false;
                }

                QTest::qSleep(1);
            }

            return recorder.messages.size() >= count;
        }
    };

    struct ClosingSession : SimConnectSession
    {
        ClosingSession() = default;
        ClosingSession(const ClosingSession&) = delete;
        ClosingSession& operator=(const ClosingSession&) = delete;
        ~ClosingSession() { Close(); }
    };

    struct PacketCase
    {
        std::function<HRESULT(HANDLE)> call;
        std::uint32_t type = 0;
        Bytes payload;
    };

}

Q_DECLARE_METATYPE(PacketCase)

class SimConnectTcpTest final : public QObject
{
    Q_OBJECT

private slots:
    static void init();
    static void cleanup();

    static void theEndpointDefaultsToLoopbackPort5111();
    static void openSendsTheHandshakePacketByteForByte();
    static void everyOutgoingCallProducesItsExactPacket_data();
    static void everyOutgoingCallProducesItsExactPacket();
    static void aTransmittedEventMatchesALiteralPacket();
    static void sendIdsCountUpFromTheOpenPacket();
    static void aBacklogLargerThanTheSocketBuffersArrivesWholeAndInOrder();
    static void aBacklogBeyondTheCapFailsTheCall();

    static void dispatchHandsEachKindToTheCallbackWithItsFields_data();
    static void dispatchHandsEachKindToTheCallbackWithItsFields();
    static void dispatchPassesUnknownKindsThrough();
    static void dispatchWithNothingToReadReturnsAtOnce();
    static void aPacketSplitByteByByteIsDeliveredOnceComplete();
    static void packetsGluedTogetherAreDeliveredSeparately();
    static void multiKilobyteClientDataArrivesIntact();
    static void aBurstOfClientDataIsDrainedByOneDispatch();
    static void anImplausibleFrameSizeIsALostConnection();

    static void openFailsFastWhenNothingListens();
    static void openGivesUpWithinTheConnectTimeout();
    static void peerCloseSurfacesAsOneQuit();
    static void aQuitFromTheSimulatorIsNotDuplicatedByTheClose();
    static void dataBufferedBeforeTheCloseIsStillDelivered();
    static void noOpenReplyWithinTheDeadlineFailsDispatch();
    static void anOpenReplyInTimeKeepsDispatchHealthy();
    static void closingFromInsideTheCallbackIsSafe();
    static void staleAndNullArgumentsAreRejected();
    static void aClosedConnectionCanBeReopened();

    static void sessionSubscribesOverTheWireAndRoutesTicks();
    static void sessionReportsPeerCloseAsQuitAndDispatchSucceeds();
    static void sessionKeepsWorkingAfterAnUnrecognizedIdException();
    static void sessionRequestsClientDataAndHandsOverTheArea();
    static void gatewayRegistersAndReadsAnLVarOverTheWire();
    static void gatewayWritesAnLVarAsASetDataPacket();
};

void SimConnectTcpTest::init()
{
    ResetSimConnectTcpSettings();
}

void SimConnectTcpTest::cleanup()
{
    ResetSimConnectTcpSettings();
}

void SimConnectTcpTest::theEndpointDefaultsToLoopbackPort5111()
{
    const SimConnectTcpEndpoint endpoint = GetSimConnectTcpEndpoint();

    QCOMPARE(endpoint.host, std::string("127.0.0.1"));
    QCOMPARE(endpoint.port, std::uint16_t{5111});

    SetSimConnectTcpEndpoint("10.0.0.7", 6000);

    QCOMPARE(GetSimConnectTcpEndpoint().host, std::string("10.0.0.7"));
    QCOMPARE(GetSimConnectTcpEndpoint().port, std::uint16_t{6000});
}

void SimConnectTcpTest::openSendsTheHandshakePacketByteForByte()
{
    Rig rig;

    QVERIFY(rig.Connect());

    const Bytes expected = Join({U32(296), U32(5), U32(0xF0000001u), U32(0), Text("GSX Integrator", 256),
                                 U32(0), Bytes{0}, Bytes{'H', 'K', 0}, U32(11), U32(0), U32(62651), U32(3)});

    QCOMPARE(rig.openPacket.size(), std::size_t{296});
    QCOMPARE(Hex(rig.openPacket), Hex(expected));
}

void SimConnectTcpTest::everyOutgoingCallProducesItsExactPacket_data()
{
    QTest::addColumn<PacketCase>("packet");

    constexpr std::uint32_t kClientEvent = 0x1000;
    constexpr std::uint32_t kAreaId = 0x47535801;
    constexpr std::uint32_t kDefId = 0x47535803;
    constexpr std::uint32_t kRequestId = 0x47535805;
    constexpr double kValue = 42.5;
    static const char kHello[] = {'h', 'e', 'l', 'l', 'o'};
    static double value = kValue;

    QTest::newRow("map client event to sim event")
        << PacketCase{[](HANDLE handle) { return SimConnect_MapClientEventToSimEvent(handle, kClientEvent, "TOGGLE_JETWAY"); },
                      kMapClientEventType, Join({U32(kClientEvent), Text("TOGGLE_JETWAY", kNameField)})};

    QTest::newRow("transmit client event")
        << PacketCase{[](HANDLE handle)
                      {
                          return SimConnect_TransmitClientEvent(handle, SIMCONNECT_OBJECT_ID_USER, kClientEvent, 7,
                                                                SIMCONNECT_GROUP_PRIORITY_HIGHEST,
                                                                SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                      },
                      kTransmitClientEventType, Join({U32(0), U32(kClientEvent), U32(7), U32(1), U32(0x10)})};

    QTest::newRow("add client event to notification group")
        << PacketCase{[](HANDLE handle) { return SimConnect_AddClientEventToNotificationGroup(handle, 1, 5); },
                      kAddToNotificationGroupType, Join({U32(1), U32(5), U32(0)})};

    QTest::newRow("add maskable client event to notification group")
        << PacketCase{[](HANDLE handle) { return SimConnect_AddClientEventToNotificationGroup(handle, 1, 5, 1); },
                      kAddToNotificationGroupType, Join({U32(1), U32(5), U32(1)})};

    QTest::newRow("set notification group priority")
        << PacketCase{[](HANDLE handle)
                      {
                          return SimConnect_SetNotificationGroupPriority(handle, 1, SIMCONNECT_GROUP_PRIORITY_HIGHEST);
                      },
                      kSetGroupPriorityType, Join({U32(1), U32(1)})};

    QTest::newRow("subscribe to system event")
        << PacketCase{[](HANDLE handle) { return SimConnect_SubscribeToSystemEvent(handle, 6, "Pause_EX1"); },
                      kSubscribeToSystemEventType, Join({U32(6), Text("Pause_EX1", kNameField)})};

    QTest::newRow("request system state")
        << PacketCase{[](HANDLE handle) { return SimConnect_RequestSystemState(handle, 0x0FFFFFFF, "Sim"); },
                      kRequestSystemStateType, Join({U32(0x0FFFFFFF), Text("Sim", kNameField)})};

    QTest::newRow("add numeric datum to data definition")
        << PacketCase{[](HANDLE handle)
                      {
                          return SimConnect_AddToDataDefinition(handle, 3, "L:FSDT_GSX_COUATL_STARTED", "Number",
                                                                SIMCONNECT_DATATYPE_FLOAT64);
                      },
                      kAddToDataDefinitionType,
                      Join({U32(3), Text("L:FSDT_GSX_COUATL_STARTED", kNameField), Text("Number", kNameField), U32(4),
                            F32(0.0F), U32(kUnused)})};

    QTest::newRow("add string datum without units to data definition")
        << PacketCase{[](HANDLE handle)
                      {
                          return SimConnect_AddToDataDefinition(handle, 4, "TITLE", nullptr,
                                                                SIMCONNECT_DATATYPE_STRING256);
                      },
                      kAddToDataDefinitionType,
                      Join({U32(4), Text("TITLE", kNameField), Text("", kNameField), U32(9), F32(0.0F),
                            U32(kUnused)})};

    QTest::newRow("clear data definition")
        << PacketCase{[](HANDLE handle) { return SimConnect_ClearDataDefinition(handle, 3); },
                      kClearDataDefinitionType, Join({U32(3)})};

    QTest::newRow("request data on sim object")
        << PacketCase{[](HANDLE handle)
                      {
                          return SimConnect_RequestDataOnSimObject(handle, 3, 3, SIMCONNECT_OBJECT_ID_USER,
                                                                   SIMCONNECT_PERIOD_SIM_FRAME,
                                                                   SIMCONNECT_DATA_REQUEST_FLAG_CHANGED, 0, 1);
                      },
                      kRequestDataOnSimObjectType,
                      Join({U32(3), U32(3), U32(0), U32(3), U32(1), U32(0), U32(1), U32(0)})};

    QTest::newRow("set data on sim object")
        << PacketCase{[](HANDLE handle)
                      {
                          return SimConnect_SetDataOnSimObject(handle, 3, SIMCONNECT_OBJECT_ID_USER, 0, 0,
                                                               sizeof(value), &value);
                      },
                      kSetDataOnSimObjectType,
                      Join({U32(3), U32(0), U32(0), U32(1), U32(8), Bytes{0, 0, 0, 0, 0, 0x40, 0x45, 0x40}})};

    QTest::newRow("map client data name to id")
        << PacketCase{[](HANDLE handle) { return SimConnect_MapClientDataNameToID(handle, "GSXI.CommBus.Tx", kAreaId); },
                      kMapClientDataNameType, Join({Text("GSXI.CommBus.Tx", kNameField), U32(kAreaId)})};

    QTest::newRow("add to client data definition")
        << PacketCase{[](HANDLE handle) { return SimConnect_AddToClientDataDefinition(handle, kDefId, 0, 8192, 0, 0); },
                      kAddToClientDataDefinitionType, Join({U32(kDefId), U32(0), U32(8192), F32(0.0F), U32(0)})};

    QTest::newRow("request client data")
        << PacketCase{[](HANDLE handle)
                      {
                          return SimConnect_RequestClientData(handle, kAreaId, kRequestId, kDefId,
                                                              SIMCONNECT_CLIENT_DATA_PERIOD_VISUAL_FRAME,
                                                              SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED, 0, 0, 0);
                      },
                      kRequestClientDataType,
                      Join({U32(kAreaId), U32(kRequestId), U32(kDefId), U32(2), U32(1), U32(0), U32(0), U32(0)})};

    QTest::newRow("set client data")
        << PacketCase{[](HANDLE handle)
                      {
                          return SimConnect_SetClientData(handle, kAreaId, kDefId,
                                                          SIMCONNECT_CLIENT_DATA_SET_FLAG_DEFAULT, 0,
                                                          sizeof(kHello), const_cast<char*>(kHello));
                      },
                      kSetClientDataType,
                      Join({U32(kAreaId), U32(kDefId), U32(0), U32(1), U32(5), Bytes{'h', 'e', 'l', 'l', 'o'}})};
}

void SimConnectTcpTest::everyOutgoingCallProducesItsExactPacket()
{
    QFETCH(PacketCase, packet);

    Rig rig;
    QVERIFY(rig.Connect());

    QVERIFY(SUCCEEDED(packet.call(rig.handle)));

    const Bytes expected = Outgoing(packet.type, 1, packet.payload);

    QCOMPARE(Hex(rig.simulator.ReadPacket()), Hex(expected));
}

void SimConnectTcpTest::aTransmittedEventMatchesALiteralPacket()
{
    Rig rig;
    QVERIFY(rig.Connect());

    QVERIFY(SUCCEEDED(SimConnect_TransmitClientEvent(rig.handle, SIMCONNECT_OBJECT_ID_USER, 0x1000, 7,
                                                     SIMCONNECT_GROUP_PRIORITY_HIGHEST,
                                                     SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY)));

    const Bytes literal = {0x24, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0xF0,
                           0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
                           0x07, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00};

    QCOMPARE(Hex(rig.simulator.ReadPacket()), Hex(literal));
}

void SimConnectTcpTest::sendIdsCountUpFromTheOpenPacket()
{
    Rig rig;
    QVERIFY(rig.Connect());

    QCOMPARE(Word(rig.openPacket, 12), std::uint32_t{0});

    for (std::uint32_t expectedId = 1; expectedId <= 3; ++expectedId)
    {
        QVERIFY(SUCCEEDED(SimConnect_ClearDataDefinition(rig.handle, expectedId)));

        const Bytes packet = rig.simulator.ReadPacket();

        QCOMPARE(Word(packet, 12), expectedId);
    }
}

void SimConnectTcpTest::aBacklogLargerThanTheSocketBuffersArrivesWholeAndInOrder()
{
    Rig rig;
    QVERIFY(rig.Connect());

    constexpr std::uint32_t kWrites = 700;
    const Bytes area = Pattern(kClientDataSize, 7);
    Bytes expected;
    for (std::uint32_t index = 0; index < kWrites; ++index)
    {
        Bytes payload = Join({U32(0x47535801), U32(index), U32(0), U32(1), U32(kClientDataSize), area});
        const Bytes packet = Outgoing(kSetClientDataType, index + 1, payload);
        expected.insert(expected.end(), packet.begin(), packet.end());

        QVERIFY(SUCCEEDED(SimConnect_SetClientData(rig.handle, 0x47535801, index, 0, 0, kClientDataSize,
                                                   const_cast<std::uint8_t*>(area.data()))));
    }

    Bytes received;
    Recorder recorder;
    const Clock::time_point start = Clock::now();
    while (received.size() < expected.size() && ElapsedMs(start) < 10000)
    {
        QVERIFY(SUCCEEDED(rig.Dispatch(recorder)));
        rig.simulator.ReadSome(received, 1u << 16, 5);
    }

    QCOMPARE(received.size(), expected.size());
    QVERIFY(received == expected);
}

void SimConnectTcpTest::aBacklogBeyondTheCapFailsTheCall()
{
    Rig rig;
    QVERIFY(rig.Connect());

    const Bytes area = Pattern(kClientDataSize, 3);
    bool failed = false;
    for (int write = 0; write < 4000 && !failed; ++write)
    {
        failed = FAILED(SimConnect_SetClientData(rig.handle, 1, 1, 0, 0, kClientDataSize,
                                                 const_cast<std::uint8_t*>(area.data())));
    }

    QVERIFY(failed);
}

void SimConnectTcpTest::dispatchHandsEachKindToTheCallbackWithItsFields_data()
{
    QTest::addColumn<Bytes>("packet");
    QTest::addColumn<std::uint32_t>("id");

    QTest::newRow("open") << OpenReply() << kRecvOpen;
    QTest::newRow("event") << EventPacket(3, 1) << kRecvEvent;
    QTest::newRow("system state")
        << Incoming(kRecvSystemState, Join({U32(0x0FFFFFFF), U32(1), F32(0.0F), Text("", 260)})) << kRecvSystemState;
    QTest::newRow("sim object data") << SimObjectDouble(5, 1.25) << kRecvSimObjectData;
    QTest::newRow("client data") << ClientDataPacket(9, Bytes{1, 2, 3, 4, 5}) << kRecvClientData;
    QTest::newRow("exception")
        << Incoming(kRecvException, Join({U32(kUnrecognizedIdException), U32(2), U32(kUnused)})) << kRecvException;
    QTest::newRow("quit") << Incoming(kRecvQuit, Bytes{}) << kRecvQuit;
}

void SimConnectTcpTest::dispatchHandsEachKindToTheCallbackWithItsFields()
{
    QFETCH(Bytes, packet);
    QFETCH(std::uint32_t, id);

    Rig rig;
    QVERIFY(rig.Connect());
    rig.simulator.Send(packet);

    Recorder recorder;
    QVERIFY(rig.DispatchUntil(recorder, 1));

    QCOMPARE(recorder.messages.size(), std::size_t{1});
    QCOMPARE(recorder.messages[0].id, id);
    QCOMPARE(recorder.messages[0].declaredSize, static_cast<std::uint32_t>(packet.size()));
    QVERIFY(recorder.messages[0].bytes == packet);
}

void SimConnectTcpTest::dispatchPassesUnknownKindsThrough()
{
    Rig rig;
    QVERIFY(rig.Connect());

    const Bytes unknown = Incoming(30, Bytes{9, 8, 7, 6, 5, 4, 3, 2});
    rig.simulator.Send(Join({unknown, EventPacket(1, 0)}));

    Recorder recorder;
    QVERIFY(rig.DispatchUntil(recorder, 2));

    QCOMPARE(recorder.messages[0].id, std::uint32_t{30});
    QCOMPARE(recorder.messages[0].declaredSize, std::uint32_t{20});
    QCOMPARE(recorder.messages[1].id, kRecvEvent);
}

void SimConnectTcpTest::dispatchWithNothingToReadReturnsAtOnce()
{
    Rig rig;
    QVERIFY(rig.Connect());

    Recorder recorder;
    const Clock::time_point start = Clock::now();

    QVERIFY(SUCCEEDED(rig.Dispatch(recorder)));
    QVERIFY(SUCCEEDED(rig.Dispatch(recorder)));

    QVERIFY(ElapsedMs(start) < 50);
    QVERIFY(recorder.messages.empty());
}

void SimConnectTcpTest::aPacketSplitByteByByteIsDeliveredOnceComplete()
{
    Rig rig;
    QVERIFY(rig.Connect());

    const Bytes packet = SimObjectDouble(7, 123.5);
    Recorder recorder;
    for (std::size_t index = 0; index + 1 < packet.size(); ++index)
    {
        rig.simulator.Send(Bytes{packet[index]});
        QTest::qSleep(1);

        QVERIFY(SUCCEEDED(rig.Dispatch(recorder)));
        QVERIFY(recorder.messages.empty());
    }

    rig.simulator.Send(Bytes{packet.back()});

    QVERIFY(rig.DispatchUntil(recorder, 1));
    QCOMPARE(recorder.messages.size(), std::size_t{1});
    QVERIFY(recorder.messages[0].bytes == packet);
}

void SimConnectTcpTest::packetsGluedTogetherAreDeliveredSeparately()
{
    Rig rig;
    QVERIFY(rig.Connect());

    const Bytes first = EventPacket(1, 0);
    const Bytes second = SimObjectDouble(2, 2.0);
    const Bytes third = EventPacket(2, 0);
    const Bytes glued = Join({first, second, third});
    rig.simulator.Send(Bytes(glued.begin(), glued.begin() + static_cast<std::ptrdiff_t>(first.size() + 5)));
    QTest::qSleep(kAMoment);
    rig.simulator.Send(Bytes(glued.begin() + static_cast<std::ptrdiff_t>(first.size() + 5), glued.end()));

    Recorder recorder;
    QVERIFY(rig.DispatchUntil(recorder, 3));

    QVERIFY(recorder.messages[0].bytes == first);
    QVERIFY(recorder.messages[1].bytes == second);
    QVERIFY(recorder.messages[2].bytes == third);
}

void SimConnectTcpTest::multiKilobyteClientDataArrivesIntact()
{
    Rig rig;
    QVERIFY(rig.Connect());

    const Bytes area = Pattern(kClientDataSize, 11);
    const Bytes packet = ClientDataPacket(0x47535805, area);
    rig.simulator.Send(packet);

    Recorder recorder;
    QVERIFY(rig.DispatchUntil(recorder, 1));

    const Message& message = recorder.messages[0];

    QCOMPARE(message.id, kRecvClientData);
    QCOMPARE(message.bytes.size(), kReceiveHeaderSize + 28 + kClientDataSize);

    const auto* header = reinterpret_cast<const SIMCONNECT_RECV_CLIENT_DATA*>(message.bytes.data());

    QCOMPARE(header->dwRequestID, std::uint32_t{0x47535805});
    QCOMPARE(message.declaredSize - offsetof(SIMCONNECT_RECV_CLIENT_DATA, dwData), kClientDataSize);
    QVERIFY(std::equal(area.begin(), area.end(), message.bytes.begin() + kReceiveHeaderSize + 28));
}

void SimConnectTcpTest::aBurstOfClientDataIsDrainedByOneDispatch()
{
    Rig rig;
    QVERIFY(rig.Connect());

    constexpr std::size_t kPackets = 16;
    Bytes burst;
    for (std::size_t index = 0; index < kPackets; ++index)
    {
        const Bytes packet = ClientDataPacket(static_cast<std::uint32_t>(index),
                                              Pattern(kClientDataSize, static_cast<std::uint8_t>(index)));
        burst.insert(burst.end(), packet.begin(), packet.end());
    }

    rig.simulator.Send(burst);
    QTest::qSleep(100);

    Recorder recorder;

    QVERIFY(SUCCEEDED(rig.Dispatch(recorder)));
    QCOMPARE(recorder.messages.size(), kPackets);

    for (std::size_t index = 0; index < kPackets; ++index)
    {
        const auto* header = reinterpret_cast<const SIMCONNECT_RECV_CLIENT_DATA*>(recorder.messages[index].bytes.data());

        QCOMPARE(header->dwRequestID, static_cast<std::uint32_t>(index));
    }
}

void SimConnectTcpTest::anImplausibleFrameSizeIsALostConnection()
{
    Rig rig;
    QVERIFY(rig.Connect());

    rig.simulator.Send(Join({U32(5), U32(5), U32(kRecvEvent)}));

    Recorder recorder;
    QVERIFY(rig.DispatchUntil(recorder, 1));

    QCOMPARE(recorder.messages[0].id, kRecvQuit);
    QVERIFY(FAILED(rig.Dispatch(recorder)));
}

void SimConnectTcpTest::openFailsFastWhenNothingListens()
{
    std::uint16_t deadPort = 0;
    {
        FakeSimulator vanished;
        deadPort = vanished.Port();
    }

    SetSimConnectTcpEndpoint("127.0.0.1", deadPort);

    HANDLE handle = reinterpret_cast<HANDLE>(0x1);
    const Clock::time_point start = Clock::now();
    const HRESULT result = SimConnect_Open(&handle, "GSX Integrator", nullptr, 0, nullptr, 0);

    QVERIFY(FAILED(result));
    QVERIFY(handle == nullptr);
    QVERIFY(ElapsedMs(start) < 500);
}

void SimConnectTcpTest::openGivesUpWithinTheConnectTimeout()
{
    SetSimConnectTcpEndpoint("10.255.255.1", 5111);
    SetSimConnectTcpTimeouts(300, 5000);

    HANDLE handle = nullptr;
    const Clock::time_point start = Clock::now();
    const HRESULT result = SimConnect_Open(&handle, "GSX Integrator", nullptr, 0, nullptr, 0);

    QVERIFY(FAILED(result));
    QVERIFY(ElapsedMs(start) < 1500);
}

void SimConnectTcpTest::peerCloseSurfacesAsOneQuit()
{
    Rig rig;
    QVERIFY(rig.Connect());

    rig.simulator.CloseClient();

    Recorder recorder;
    QVERIFY(rig.DispatchUntil(recorder, 1));

    QCOMPARE(recorder.messages.size(), std::size_t{1});
    QCOMPARE(recorder.messages[0].id, kRecvQuit);
    QCOMPARE(recorder.messages[0].declaredSize, std::uint32_t{kReceiveHeaderSize});

    QVERIFY(FAILED(rig.Dispatch(recorder)));
    QCOMPARE(recorder.messages.size(), std::size_t{1});
}

void SimConnectTcpTest::aQuitFromTheSimulatorIsNotDuplicatedByTheClose()
{
    Rig rig;
    QVERIFY(rig.Connect());

    rig.simulator.Send(Incoming(kRecvQuit, Bytes{}));
    rig.simulator.CloseClient();

    Recorder recorder;
    QVERIFY(rig.DispatchUntil(recorder, 1));

    for (int round = 0; round < 5; ++round)
    {
        static_cast<void>(rig.Dispatch(recorder));
        QTest::qSleep(kAMoment);
    }

    QCOMPARE(recorder.messages.size(), std::size_t{1});
}

void SimConnectTcpTest::dataBufferedBeforeTheCloseIsStillDelivered()
{
    Rig rig;
    QVERIFY(rig.Connect());

    rig.simulator.Send(Join({EventPacket(1, 0), SimObjectDouble(4, 8.0)}));
    rig.simulator.CloseClient();
    QTest::qSleep(kAMoment);

    Recorder recorder;

    QVERIFY(SUCCEEDED(rig.Dispatch(recorder)));
    QCOMPARE(recorder.messages.size(), std::size_t{3});
    QCOMPARE(recorder.messages[0].id, kRecvEvent);
    QCOMPARE(recorder.messages[1].id, kRecvSimObjectData);
    QCOMPARE(recorder.messages[2].id, kRecvQuit);
}

void SimConnectTcpTest::noOpenReplyWithinTheDeadlineFailsDispatch()
{
    SetSimConnectTcpTimeouts(1500, 250);

    Rig rig;
    QVERIFY(rig.Connect());

    Recorder recorder;

    QVERIFY(SUCCEEDED(rig.Dispatch(recorder)));

    QTest::qSleep(300);

    QVERIFY(FAILED(rig.Dispatch(recorder)));
    QVERIFY(recorder.messages.empty());
}

void SimConnectTcpTest::anOpenReplyInTimeKeepsDispatchHealthy()
{
    SetSimConnectTcpTimeouts(1500, 250);

    Rig rig;
    QVERIFY(rig.Connect());

    rig.simulator.Send(OpenReply());

    Recorder recorder;
    QVERIFY(rig.DispatchUntil(recorder, 1));

    QTest::qSleep(300);

    QVERIFY(SUCCEEDED(rig.Dispatch(recorder)));
    QCOMPARE(recorder.messages.size(), std::size_t{1});
}

void SimConnectTcpTest::closingFromInsideTheCallbackIsSafe()
{
    Rig rig;
    QVERIFY(rig.Connect());

    rig.simulator.Send(Join({EventPacket(1, 0), EventPacket(2, 0), EventPacket(3, 0)}));
    QTest::qSleep(kAMoment);

    Recorder recorder;
    recorder.onMessage = [&rig]
    {
        if (rig.handle != nullptr)
        {
            SimConnect_Close(rig.handle);
            rig.handle = nullptr;
        }
    };

    HANDLE handle = rig.handle;

    QVERIFY(SUCCEEDED(SimConnect_CallDispatch(handle, &Recorder::Collect, &recorder)));
    QCOMPARE(recorder.messages.size(), std::size_t{1});
    QCOMPARE(SimConnect_CallDispatch(handle, &Recorder::Collect, &recorder), E_HANDLE);
}

void SimConnectTcpTest::staleAndNullArgumentsAreRejected()
{
    Rig rig;
    QVERIFY(rig.Connect());

    HANDLE stale = rig.handle;
    QVERIFY(SUCCEEDED(SimConnect_Close(stale)));
    rig.handle = nullptr;

    Recorder recorder;

    QCOMPARE(SimConnect_Close(stale), E_HANDLE);
    QCOMPARE(SimConnect_ClearDataDefinition(stale, 1), E_HANDLE);
    QCOMPARE(SimConnect_CallDispatch(stale, &Recorder::Collect, &recorder), E_HANDLE);
    QCOMPARE(SimConnect_ClearDataDefinition(nullptr, 1), E_HANDLE);
    QCOMPARE(SimConnect_Open(nullptr, "x", nullptr, 0, nullptr, 0), E_INVALIDARG);

    Rig second;
    QVERIFY(second.Connect());

    QCOMPARE(SimConnect_CallDispatch(second.handle, nullptr, &recorder), E_INVALIDARG);
    QCOMPARE(SimConnect_SetClientData(second.handle, 1, 1, 0, 0, 4, nullptr), E_INVALIDARG);
}

void SimConnectTcpTest::aClosedConnectionCanBeReopened()
{
    Rig rig;
    QVERIFY(rig.Connect());

    rig.simulator.CloseClient();

    Recorder recorder;
    QVERIFY(rig.DispatchUntil(recorder, 1));

    QVERIFY(SUCCEEDED(SimConnect_Close(rig.handle)));
    rig.handle = nullptr;

    QVERIFY(rig.Connect());
    QVERIFY(SUCCEEDED(SimConnect_ClearDataDefinition(rig.handle, 1)));

    QCOMPARE(Word(rig.simulator.ReadPacket(), 12), std::uint32_t{1});
}

void SimConnectTcpTest::sessionSubscribesOverTheWireAndRoutesTicks()
{
    Rig rig;
    ClosingSession session;

    QVERIFY(session.Open("GSX Integrator"));
    QVERIFY(rig.simulator.Accept());
    QCOMPARE(rig.simulator.ReadPacket().size(), std::size_t{296});

    int oneSecond = 0;
    int fourSecond = 0;
    QVERIFY(session.SubscribeOneSecond([&oneSecond] { ++oneSecond; }));
    QVERIFY(session.SubscribeFourSeconds([&fourSecond] { ++fourSecond; }));

    QCOMPARE(Hex(rig.simulator.ReadPacket()),
             Hex(Outgoing(kSubscribeToSystemEventType, 1, Join({U32(1), Text("1sec", 256)}))));
    QCOMPARE(Hex(rig.simulator.ReadPacket()),
             Hex(Outgoing(kSubscribeToSystemEventType, 2, Join({U32(2), Text("4sec", 256)}))));

    rig.simulator.Send(Join({OpenReply(), EventPacket(1, 0), EventPacket(2, 0), EventPacket(1, 0)}));

    const Clock::time_point start = Clock::now();
    while (oneSecond < 2 && ElapsedMs(start) < kPatient)
    {
        QVERIFY(session.Dispatch());
        QTest::qSleep(1);
    }

    QCOMPARE(oneSecond, 2);
    QCOMPARE(fourSecond, 1);
}

void SimConnectTcpTest::sessionReportsPeerCloseAsQuitAndDispatchSucceeds()
{
    Rig rig;
    ClosingSession session;

    QVERIFY(session.Open("GSX Integrator"));
    QVERIFY(rig.simulator.Accept());

    int quits = 0;
    session.SetOnQuit([&quits] { ++quits; });
    rig.simulator.CloseClient();

    const Clock::time_point start = Clock::now();
    while (quits == 0 && ElapsedMs(start) < kPatient)
    {
        QVERIFY(session.Dispatch());
        QTest::qSleep(1);
    }

    QCOMPARE(quits, 1);
}

void SimConnectTcpTest::sessionKeepsWorkingAfterAnUnrecognizedIdException()
{
    Rig rig;
    ClosingSession session;
    SimConnectVariableGateway gateway;

    QVERIFY(session.Open("GSX Integrator"));
    QVERIFY(rig.simulator.Accept());
    static_cast<void>(rig.simulator.ReadPacket());

    session.SetVarManager(&gateway);
    gateway.Attach(session.Handle());

    int ticks = 0;
    QVERIFY(session.SubscribeOneSecond([&ticks] { ++ticks; }));

    rig.simulator.Send(Join({Incoming(kRecvException, Join({U32(kUnrecognizedIdException), U32(2), U32(0)})),
                             EventPacket(1, 0)}));

    const Clock::time_point start = Clock::now();
    while (ticks == 0 && ElapsedMs(start) < kPatient)
    {
        QVERIFY(session.Dispatch());
        QTest::qSleep(1);
    }

    QCOMPARE(ticks, 1);
    QVERIFY(session.IsConnected());
}

void SimConnectTcpTest::sessionRequestsClientDataAndHandsOverTheArea()
{
    Rig rig;
    ClosingSession session;

    QVERIFY(session.Open("GSX Integrator"));
    QVERIFY(rig.simulator.Accept());
    static_cast<void>(rig.simulator.ReadPacket());

    Bytes received;
    QVERIFY(session.RequestClientDataArea("GSXI.CommBus.Rx", 0x47535802, 0x47535804, 0x47535805, kClientDataSize,
                                          SIMCONNECT_CLIENT_DATA_PERIOD_VISUAL_FRAME,
                                          SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED,
                                          [&received](const void* data, const DWORD size)
                                          {
                                              const auto* raw = static_cast<const std::uint8_t*>(data);
                                              received.assign(raw, raw + size);
                                          }));

    QCOMPARE(Hex(rig.simulator.ReadPacket()),
             Hex(Outgoing(kMapClientDataNameType, 1, Join({Text("GSXI.CommBus.Rx", 256), U32(0x47535802)}))));
    QCOMPARE(Hex(rig.simulator.ReadPacket()),
             Hex(Outgoing(kAddToClientDataDefinitionType, 2, Join({U32(0x47535804), U32(0), U32(8192), F32(0.0F), U32(0)}))));
    QCOMPARE(Hex(rig.simulator.ReadPacket()),
             Hex(Outgoing(kRequestClientDataType, 3,
                      Join({U32(0x47535802), U32(0x47535805), U32(0x47535804), U32(2), U32(1), U32(0), U32(0), U32(0)}))));

    const Bytes area = Pattern(kClientDataSize, 21);
    rig.simulator.Send(ClientDataPacket(0x47535805, area));

    const Clock::time_point start = Clock::now();
    while (received.empty() && ElapsedMs(start) < kPatient)
    {
        QVERIFY(session.Dispatch());
        QTest::qSleep(1);
    }

    QVERIFY(received == area);
}

void SimConnectTcpTest::gatewayRegistersAndReadsAnLVarOverTheWire()
{
    Rig rig;
    ClosingSession session;
    SimConnectVariableGateway gateway;

    QVERIFY(session.Open("GSX Integrator"));
    QVERIFY(rig.simulator.Accept());
    static_cast<void>(rig.simulator.ReadPacket());

    session.SetVarManager(&gateway);
    gateway.Attach(session.Handle());

    QCOMPARE(gateway.GetLVar("FSDT_GSX_COUATL_STARTED", -1.0), -1.0);

    QCOMPARE(Hex(rig.simulator.ReadPacket()),
             Hex(Outgoing(kAddToDataDefinitionType, 1,
                      Join({U32(1), Text("L:FSDT_GSX_COUATL_STARTED", 256), Text("Number", 256), U32(4), F32(0.0F),
                            U32(kUnused)}))));
    QCOMPARE(Hex(rig.simulator.ReadPacket()),
             Hex(Outgoing(kRequestDataOnSimObjectType, 2,
                      Join({U32(1), U32(1), U32(0), U32(4), U32(1), U32(0), U32(0), U32(0)}))));

    rig.simulator.Send(SimObjectDouble(1, 1.0));

    const Clock::time_point start = Clock::now();
    while (!gateway.HasReceivedLVar("FSDT_GSX_COUATL_STARTED") && ElapsedMs(start) < kPatient)
    {
        QVERIFY(session.Dispatch());
        QTest::qSleep(1);
    }

    QCOMPARE(gateway.GetLVar("FSDT_GSX_COUATL_STARTED", -1.0), 1.0);
}

void SimConnectTcpTest::gatewayWritesAnLVarAsASetDataPacket()
{
    Rig rig;
    ClosingSession session;
    SimConnectVariableGateway gateway;

    QVERIFY(session.Open("GSX Integrator"));
    QVERIFY(rig.simulator.Accept());
    static_cast<void>(rig.simulator.ReadPacket());

    session.SetVarManager(&gateway);
    gateway.Attach(session.Handle());

    gateway.SetLVar("FSDT_GSX_MENU_CHOICE", 42.5);

    static_cast<void>(rig.simulator.ReadPacket());
    static_cast<void>(rig.simulator.ReadPacket());

    QCOMPARE(Hex(rig.simulator.ReadPacket()),
             Hex(Outgoing(kSetDataOnSimObjectType, 3,
                      Join({U32(1), U32(0), U32(0), U32(1), U32(8), Bytes{0, 0, 0, 0, 0, 0x40, 0x45, 0x40}}))));
}

QTEST_GUILESS_MAIN(SimConnectTcpTest)

#include "tst_simconnect_tcp.moc"

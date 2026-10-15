#ifndef GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECT_TCP_SIMCONNECTWIRE_H
#define GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECT_TCP_SIMCONNECTWIRE_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace simconnect_tcp
{
    constexpr std::uint32_t kProtocolVersion = 5;
    constexpr std::size_t kHeaderSize = 16;
    constexpr std::size_t kReceiveHeaderSize = 12;
    constexpr std::size_t kMaxFrameSize = 4u * 1024u * 1024u;
    constexpr std::size_t kMinFrameStorage = 320;

    enum class PacketType : std::uint32_t
    {
        Open = 0x01,
        MapClientEventToSimEvent = 0x04,
        TransmitClientEvent = 0x05,
        AddClientEventToNotificationGroup = 0x07,
        SetNotificationGroupPriority = 0x09,
        AddToDataDefinition = 0x0c,
        ClearDataDefinition = 0x0d,
        RequestDataOnSimObject = 0x0e,
        SetDataOnSimObject = 0x10,
        SubscribeToSystemEvent = 0x17,
        RequestSystemState = 0x35,
        MapClientDataNameToId = 0x37,
        AddToClientDataDefinition = 0x39,
        RequestClientData = 0x3b,
        SetClientData = 0x3c,
    };

    class PacketWriter
    {
    public:
        explicit PacketWriter(PacketType type);

        PacketWriter& Uint32(std::uint32_t value);
        PacketWriter& Float32(float value);
        PacketWriter& Byte(std::uint8_t value);
        PacketWriter& Text(const char* text, std::size_t fixedLength);
        PacketWriter& Bytes(const void* data, std::size_t size);

        [[nodiscard]] std::vector<std::uint8_t> Finish(std::uint32_t sendId);

    private:
        std::vector<std::uint8_t> bytes_;
    };

    struct Frame
    {
        std::vector<std::uint64_t> words;
        std::size_t size = 0;

        [[nodiscard]] const void* Data() const { return words.data(); }
        [[nodiscard]] std::uint32_t Id() const;
    };

    [[nodiscard]] Frame MakeQuitFrame();

    class FrameBuffer
    {
    public:
        void Append(const std::uint8_t* data, std::size_t size);
        [[nodiscard]] bool Take(Frame& frame);
        [[nodiscard]] bool IsCorrupt() const { return corrupt_; }
        [[nodiscard]] std::size_t Pending() const { return bytes_.size() - consumed_; }

    private:
        std::vector<std::uint8_t> bytes_;
        std::size_t consumed_ = 0;
        bool corrupt_ = false;
    };
}

#endif // GSX_INTEGRATOR_CLIENT_INFRASTRUCTURE_SIMCONNECT_TCP_SIMCONNECTWIRE_H

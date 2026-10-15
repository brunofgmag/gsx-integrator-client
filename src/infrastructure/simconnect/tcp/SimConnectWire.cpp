#include "SimConnectWire.h"

#include <algorithm>
#include <cstring>
#include <utility>
#include "SimConnectDefinitions.h"

namespace
{
    constexpr std::uint32_t kPacketTypeMask = 0xF0000000u;
    constexpr std::size_t kSizeOffset = 0;
    constexpr std::size_t kProtocolOffset = 4;
    constexpr std::size_t kTypeOffset = 8;
    constexpr std::size_t kSendIdOffset = 12;
    constexpr std::size_t kWordSize = sizeof(std::uint64_t);
    constexpr std::size_t kCompactThreshold = 1u << 16;

    void StoreUint32(std::vector<std::uint8_t>& bytes, const std::size_t offset, const std::uint32_t value)
    {
        bytes[offset] = static_cast<std::uint8_t>(value & 0xFFu);
        bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
        bytes[offset + 2] = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
        bytes[offset + 3] = static_cast<std::uint8_t>((value >> 24) & 0xFFu);
    }

    std::uint32_t LoadUint32(const std::uint8_t* bytes)
    {
        return static_cast<std::uint32_t>(bytes[0])
            | (static_cast<std::uint32_t>(bytes[1]) << 8)
            | (static_cast<std::uint32_t>(bytes[2]) << 16)
            | (static_cast<std::uint32_t>(bytes[3]) << 24);
    }
}

namespace simconnect_tcp
{
    PacketWriter::PacketWriter(const PacketType type)
        : bytes_(kHeaderSize, 0)
    {
        StoreUint32(bytes_, kProtocolOffset, kProtocolVersion);
        StoreUint32(bytes_, kTypeOffset, kPacketTypeMask | static_cast<std::uint32_t>(type));
    }

    PacketWriter& PacketWriter::Uint32(const std::uint32_t value)
    {
        const std::size_t offset = bytes_.size();
        bytes_.resize(offset + sizeof(value));
        StoreUint32(bytes_, offset, value);

        return *this;
    }

    PacketWriter& PacketWriter::Float32(const float value)
    {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));

        return Uint32(bits);
    }

    PacketWriter& PacketWriter::Byte(const std::uint8_t value)
    {
        bytes_.push_back(value);

        return *this;
    }

    PacketWriter& PacketWriter::Text(const char* text, const std::size_t fixedLength)
    {
        const std::size_t offset = bytes_.size();
        bytes_.resize(offset + fixedLength, 0);

        if (text != nullptr && fixedLength > 0)
        {
            const std::size_t length = (std::min)(std::strlen(text), fixedLength - 1);
            std::memcpy(bytes_.data() + offset, text, length);
        }

        return *this;
    }

    PacketWriter& PacketWriter::Bytes(const void* data, const std::size_t size)
    {
        if (data == nullptr || size == 0)
        {
            return *this;
        }

        const auto* source = static_cast<const std::uint8_t*>(data);
        bytes_.insert(bytes_.end(), source, source + size);

        return *this;
    }

    std::vector<std::uint8_t> PacketWriter::Finish(const std::uint32_t sendId)
    {
        StoreUint32(bytes_, kSizeOffset, static_cast<std::uint32_t>(bytes_.size()));
        StoreUint32(bytes_, kSendIdOffset, sendId);

        return std::move(bytes_);
    }

    std::uint32_t Frame::Id() const
    {
        std::uint32_t id = 0;
        std::memcpy(&id, static_cast<const std::uint8_t*>(Data()) + kTypeOffset, sizeof(id));

        return id;
    }

    Frame MakeQuitFrame()
    {
        Frame frame;
        frame.size = kReceiveHeaderSize;
        frame.words.assign(kMinFrameStorage / kWordSize, 0);

        SIMCONNECT_RECV header{};
        header.dwSize = static_cast<DWORD>(kReceiveHeaderSize);
        header.dwVersion = kProtocolVersion;
        header.dwID = SIMCONNECT_RECV_ID_QUIT;
        std::memcpy(frame.words.data(), &header, sizeof(header));

        return frame;
    }

    void FrameBuffer::Append(const std::uint8_t* data, const std::size_t size)
    {
        if (consumed_ >= kCompactThreshold && consumed_ * 2 >= bytes_.size())
        {
            bytes_.erase(bytes_.begin(), bytes_.begin() + static_cast<std::ptrdiff_t>(consumed_));
            consumed_ = 0;
        }

        bytes_.insert(bytes_.end(), data, data + size);
    }

    bool FrameBuffer::Take(Frame& frame)
    {
        if (corrupt_ || Pending() < sizeof(std::uint32_t))
        {
            return false;
        }

        const std::uint8_t* start = bytes_.data() + consumed_;
        const std::size_t size = LoadUint32(start);

        if (size < kReceiveHeaderSize || size > kMaxFrameSize)
        {
            corrupt_ = true;

            return false;
        }

        if (Pending() < size)
        {
            return false;
        }

        const std::size_t storage = (std::max)(size, kMinFrameStorage);
        frame.size = size;
        frame.words.assign((storage + kWordSize - 1) / kWordSize, 0);
        std::memcpy(frame.words.data(), start, size);
        consumed_ += size;

        if (consumed_ == bytes_.size())
        {
            bytes_.clear();
            consumed_ = 0;
        }

        return true;
    }
}

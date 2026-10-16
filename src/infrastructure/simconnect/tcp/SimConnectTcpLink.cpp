#include "SimConnectTcpLink.h"

#include <cerrno>
#include <cstring>
#include <string>
#include <utility>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace
{
    constexpr std::size_t kReadChunk = 65536;
    constexpr int kMaxReadsPerPump = 256;
    constexpr std::size_t kMaxOutgoing = 8u * 1024u * 1024u;
    constexpr int kInvalidDescriptor = -1;

    struct AddressList
    {
        addrinfo* head = nullptr;

        AddressList() = default;
        AddressList(const AddressList&) = delete;
        AddressList& operator=(const AddressList&) = delete;

        ~AddressList()
        {
            if (head != nullptr)
            {
                freeaddrinfo(head);
            }
        }
    };

    bool WaitUntilWritable(const int descriptor, const int timeoutMs)
    {
        pollfd pending{};
        pending.fd = descriptor;
        pending.events = POLLOUT;

        int ready = 0;
        do
        {
            ready = poll(&pending, 1, timeoutMs);
        }
        while (ready < 0 && errno == EINTR);

        if (ready <= 0)
        {
            return false;
        }

        int error = 0;
        socklen_t length = sizeof(error);
        if (getsockopt(descriptor, SOL_SOCKET, SO_ERROR, &error, &length) != 0)
        {
            return false;
        }

        return error == 0;
    }

    int ConnectTo(const addrinfo& address, const int timeoutMs)
    {
        const int descriptor = socket(address.ai_family, address.ai_socktype | SOCK_NONBLOCK | SOCK_CLOEXEC,
                                      address.ai_protocol);
        if (descriptor < 0)
        {
            return kInvalidDescriptor;
        }

        const int result = connect(descriptor, address.ai_addr, address.ai_addrlen);
        const bool connected = result == 0
            || (errno == EINPROGRESS && WaitUntilWritable(descriptor, timeoutMs));
        if (!connected)
        {
            close(descriptor);

            return kInvalidDescriptor;
        }

        const int enabled = 1;
        setsockopt(descriptor, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof(enabled));

        return descriptor;
    }
}

namespace simconnect_tcp
{
    std::unique_ptr<TcpLink> TcpLink::Connect(const std::string& host, const std::uint16_t port, const int timeoutMs)
    {
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        AddressList addresses;
        const std::string service = std::to_string(port);
        if (getaddrinfo(host.c_str(), service.c_str(), &hints, &addresses.head) != 0)
        {
            return nullptr;
        }

        for (const addrinfo* address = addresses.head; address != nullptr; address = address->ai_next)
        {
            if (const int descriptor = ConnectTo(*address, timeoutMs); descriptor != kInvalidDescriptor)
            {
                return std::unique_ptr<TcpLink>(new TcpLink(descriptor));
            }
        }

        return nullptr;
    }

    TcpLink::TcpLink(const int descriptor)
        : descriptor_(descriptor)
    {
    }

    TcpLink::~TcpLink()
    {
        if (descriptor_ != kInvalidDescriptor)
        {
            close(descriptor_);
        }
    }

    bool TcpLink::Send(const std::vector<std::uint8_t>& packet)
    {
        if (closed_)
        {
            return false;
        }

        if (outgoing_.size() - sent_ + packet.size() > kMaxOutgoing)
        {
            closed_ = true;

            return false;
        }

        outgoing_.insert(outgoing_.end(), packet.begin(), packet.end());
        Flush();

        return !closed_;
    }

    void TcpLink::Flush()
    {
        while (!closed_ && sent_ < outgoing_.size())
        {
            const ssize_t written = send(descriptor_, outgoing_.data() + sent_, outgoing_.size() - sent_,
                                         MSG_NOSIGNAL);
            if (written > 0)
            {
                sent_ += static_cast<std::size_t>(written);
                continue;
            }

            if (written < 0 && errno == EINTR)
            {
                continue;
            }

            if (written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                return;
            }

            closed_ = true;
        }

        if (sent_ == outgoing_.size())
        {
            outgoing_.clear();
            sent_ = 0;
        }
    }

    void TcpLink::Pump()
    {
        Flush();

        std::vector<std::uint8_t> chunk(kReadChunk);
        for (int reads = 0; reads < kMaxReadsPerPump && !closed_; ++reads)
        {
            const ssize_t received = recv(descriptor_, chunk.data(), chunk.size(), 0);
            if (received > 0)
            {
                frames_.Append(chunk.data(), static_cast<std::size_t>(received));
                continue;
            }

            if (received < 0 && errno == EINTR)
            {
                continue;
            }

            if (received < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                return;
            }

            closed_ = true;
        }
    }

    bool TcpLink::Take(Frame& frame)
    {
        return frames_.Take(frame);
    }
}

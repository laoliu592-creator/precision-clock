#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace net {
class UdpSocket {
public:
    UdpSocket() noexcept = default;
    ~UdpSocket();
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;
    bool open();
    bool enable_timestamping();
    bool send_host(const std::string& host, std::uint16_t port, const void* data, std::size_t len);
    int receive(void* data, std::size_t len, std::int64_t& kernel_rx_realtime_ns, int timeout_ms);
    void close() noexcept;
private:
    int fd_{-1};
};
}

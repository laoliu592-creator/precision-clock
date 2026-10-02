#include "udp_socket.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
namespace net {
UdpSocket::~UdpSocket() { close(); }
bool UdpSocket::open() { close(); fd_ = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP); return fd_ >= 0; }
bool UdpSocket::enable_timestamping() {
    if (fd_ < 0) return false;
#ifdef SO_TIMESTAMPNS
    int on_ns = 1;
    if (::setsockopt(fd_, SOL_SOCKET, SO_TIMESTAMPNS, &on_ns, sizeof(on_ns)) == 0) return true;
#endif
#ifdef SO_TIMESTAMP
    int on_us = 1;
    return ::setsockopt(fd_, SOL_SOCKET, SO_TIMESTAMP, &on_us, sizeof(on_us)) == 0;
#else
    return false;
#endif
}
bool UdpSocket::send_host(const std::string& host, std::uint16_t port, const void* data, std::size_t len) {
    if (fd_ < 0 || data == nullptr || len == 0U) return false;
    addrinfo hints{}; hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM; hints.ai_protocol = IPPROTO_UDP;
    addrinfo* result = nullptr; const std::string service = std::to_string(port);
    if (::getaddrinfo(host.c_str(), service.c_str(), &hints, &result) != 0 || result == nullptr) return false;
    bool ok = false;
    for (addrinfo* ai = result; ai != nullptr; ai = ai->ai_next) { const ssize_t sent = ::sendto(fd_, data, len, 0, ai->ai_addr, ai->ai_addrlen); if (sent == static_cast<ssize_t>(len)) { ok = true; break; } }
    ::freeaddrinfo(result); return ok;
}
int UdpSocket::receive(void* data, std::size_t len, std::int64_t& kernel_rx_realtime_ns, int timeout_ms) {
    kernel_rx_realtime_ns = 0; if (fd_ < 0 || data == nullptr || len == 0U) return -1;
    timeval tv{}; tv.tv_sec = timeout_ms / 1000; tv.tv_usec = (timeout_ms % 1000) * 1000; if (::setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) return -1;
    alignas(cmsghdr) unsigned char control[256]{}; iovec iov{data, len}; msghdr msg{}; msg.msg_iov = &iov; msg.msg_iovlen = 1; msg.msg_control = control; msg.msg_controllen = sizeof(control);
    const ssize_t n = ::recvmsg(fd_, &msg, 0); if (n < 0 || (msg.msg_flags & MSG_TRUNC) != 0) return -1;
    for (cmsghdr* c = CMSG_FIRSTHDR(&msg); c != nullptr; c = CMSG_NXTHDR(&msg, c)) {
#ifdef SO_TIMESTAMPNS
        if (c->cmsg_level == SOL_SOCKET && c->cmsg_type == SO_TIMESTAMPNS && c->cmsg_len >= CMSG_LEN(sizeof(timespec))) { timespec ts{}; std::memcpy(&ts, CMSG_DATA(c), sizeof(ts)); kernel_rx_realtime_ns = static_cast<std::int64_t>(ts.tv_sec) * 1'000'000'000LL + ts.tv_nsec; break; }
#endif
#ifdef SO_TIMESTAMP
        if (kernel_rx_realtime_ns == 0 && c->cmsg_level == SOL_SOCKET && c->cmsg_type == SO_TIMESTAMP && c->cmsg_len >= CMSG_LEN(sizeof(timeval))) { timeval tv2{}; std::memcpy(&tv2, CMSG_DATA(c), sizeof(tv2)); kernel_rx_realtime_ns = static_cast<std::int64_t>(tv2.tv_sec) * 1'000'000'000LL + static_cast<std::int64_t>(tv2.tv_usec) * 1000LL; }
#endif
    }
    return static_cast<int>(n);
}
void UdpSocket::close() noexcept { if (fd_ >= 0) { ::close(fd_); fd_ = -1; } }
}

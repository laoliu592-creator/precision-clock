#include "ntp_client.h"
#include "ntp_packet.h"
#include "../socket/udp_socket.h"
#include "../clock/clock_engine.h"
namespace ntp {
bool Client::sync_once(const char* server, std::uint16_t port, SyncSample& out) {
    out = {};
    if (server == nullptr) return false;
    net::UdpSocket sock;
    if (!sock.open()) return false;
    (void)sock.enable_timestamping();
    const std::int64_t t1 = clockcore::realtime_ns();
    const std::int64_t m1 = clockcore::monotonic_ns();
    if (t1 <= 0 || m1 <= 0) return false;
    const std::uint64_t tx = unix_ns_to_ntp64(t1);
    const Packet req = make_request(tx);
    if (!sock.send_host(server, port, req.bytes.data(), req.bytes.size())) return false;
    std::uint8_t response[512]{};
    std::int64_t kernel_t4 = 0;
    const int n = sock.receive(response, sizeof(response), kernel_t4, 1800);
    const std::int64_t m4 = clockcore::monotonic_ns();
    if (n < 48 || m4 <= 0) return false;
    Exchange x{};
    if (!parse_exchange(response, static_cast<std::size_t>(n), tx, x)) return false;
    const std::int64_t t4 = kernel_t4 > 0 ? kernel_t4 : clockcore::realtime_ns();
    if (t4 <= 0) return false;
    x.t4_ns = t4;
    const std::int64_t a = x.t2_ns - x.t1_ns;
    const std::int64_t b = x.t3_ns - x.t4_ns;
    const std::int64_t offset = (a + b) / 2LL;
    const std::int64_t delay = (x.t4_ns - x.t1_ns) - (x.t3_ns - x.t2_ns);
    if (delay < 0 || delay > 10'000'000'000LL) return false;
    out.offset_ns = offset;
    out.delay_ns = delay;
    out.rx_realtime_ns = t4;
    out.rx_mono_ns = m4;
    out.corrected_server_ns = t4 + offset;
    (void)m1;
    return true;
}
} // namespace ntp

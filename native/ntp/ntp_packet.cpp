#include "ntp_packet.h"
#include <limits>
namespace ntp {
namespace { constexpr std::int64_t NTP_UNIX_EPOCH = 2208988800LL; }
std::uint32_t read_be32(const std::uint8_t* p) noexcept {
    return (static_cast<std::uint32_t>(p[0]) << 24U) |
           (static_cast<std::uint32_t>(p[1]) << 16U) |
           (static_cast<std::uint32_t>(p[2]) << 8U) | p[3];
}
std::uint64_t read_be64(const std::uint8_t* p) noexcept {
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8U) | p[i];
    return v;
}
void write_be64(std::uint8_t* p, std::uint64_t v) noexcept {
    for (int i = 7; i >= 0; --i) { p[i] = static_cast<std::uint8_t>(v); v >>= 8U; }
}
std::uint64_t unix_ns_to_ntp64(std::int64_t unix_ns) noexcept {
    const std::int64_t sec = unix_ns / 1000000000LL;
    const std::int64_t rem = unix_ns % 1000000000LL;
    const std::uint64_t ntp_sec = static_cast<std::uint64_t>(sec + NTP_UNIX_EPOCH);
    const std::uint64_t positive_rem = static_cast<std::uint64_t>(rem < 0 ? rem + 1000000000LL : rem);
    const std::uint64_t frac = positive_rem * 0x100000000ULL / 1000000000ULL;
    return (ntp_sec << 32U) | frac;
}
std::int64_t ntp64_to_unix_ns(std::uint64_t ntp) noexcept {
    const std::uint64_t sec = ntp >> 32U;
    const std::uint64_t frac = ntp & 0xffffffffULL;
    if (sec < static_cast<std::uint64_t>(NTP_UNIX_EPOCH)) return 0;
    const std::uint64_t unix_sec = sec - static_cast<std::uint64_t>(NTP_UNIX_EPOCH);
    const std::uint64_t ns = (frac * 1000000000ULL) >> 32U;
    if (unix_sec > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max() / 1000000000LL)) return 0;
    return static_cast<std::int64_t>(unix_sec * 1000000000ULL + ns);
}
Packet make_request(std::uint64_t client_tx_timestamp) noexcept {
    Packet p{};
    p.bytes[0] = static_cast<std::uint8_t>((4U << 3U) | 3U);
    write_be64(p.bytes.data() + 40, client_tx_timestamp);
    return p;
}
bool valid_response(const std::uint8_t* data, std::size_t len, std::uint64_t request_tx) noexcept {
    if (data == nullptr || len < 48U) return false;
    const std::uint8_t mode = data[0] & 7U;
    const std::uint8_t ver = (data[0] >> 3U) & 7U;
    if ((mode != 4U && mode != 5U) || ver < 3U || ver > 4U) return false;
    const std::uint8_t li = (data[0] >> 6U) & 3U;
    if (li == 3U) return false;
    const std::uint8_t stratum = data[1];
    if (stratum == 0U || stratum > 15U) return false;
    if (read_be64(data + 24) != request_tx) return false;
    if (read_be64(data + 32) == 0U || read_be64(data + 40) == 0U) return false;
    return true;
}
bool parse_exchange(const std::uint8_t* data, std::size_t len, std::uint64_t request_tx, Exchange& out) noexcept {
    if (!valid_response(data, len, request_tx)) return false;
    out.t1_ns = ntp64_to_unix_ns(request_tx);
    out.t2_ns = ntp64_to_unix_ns(read_be64(data + 32));
    out.t3_ns = ntp64_to_unix_ns(read_be64(data + 40));
    return out.t1_ns > 0 && out.t2_ns > 0 && out.t3_ns > 0;
}
}

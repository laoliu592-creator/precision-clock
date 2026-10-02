#pragma once
#include <cstdint>
namespace ntp {
struct SyncSample {
    std::int64_t offset_ns{};
    std::int64_t delay_ns{};
    std::int64_t corrected_server_ns{};
    std::int64_t rx_realtime_ns{};
    std::int64_t rx_mono_ns{};
};
class Client {
public:
    bool sync_once(const char* server, std::uint16_t port, SyncSample& out);
};
}

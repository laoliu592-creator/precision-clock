#pragma once
#include <atomic>
#include <cstdint>
namespace clockcore {
struct ClockSnapshot {
    int64_t offset_ns{0};
    int64_t sync_mono_ns{0};
    int64_t sync_server_ns{0};
    int64_t rtt_ns{0};
    int64_t jitter_ns{0};
    uint32_t samples{0};
    uint32_t status{0};
};
class ClockState {
public:
    void publish(const ClockSnapshot& snapshot) noexcept;
    ClockSnapshot snapshot() const noexcept;
    void set_status(uint32_t status) noexcept { status_.store(status, std::memory_order_release); }
private:
    std::atomic<int64_t> offset_ns_{0};
    std::atomic<int64_t> sync_mono_ns_{0};
    std::atomic<int64_t> sync_server_ns_{0};
    std::atomic<int64_t> rtt_ns_{0};
    std::atomic<int64_t> jitter_ns_{0};
    std::atomic<uint32_t> samples_{0};
    std::atomic<uint32_t> status_{0};
};
} // namespace clockcore

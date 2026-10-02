#pragma once
#include "clock_state.h"
#include <cstdint>
namespace clockcore {
int64_t monotonic_ns() noexcept;
int64_t realtime_ns() noexcept;
class ClockEngine {
public:
    ClockEngine() = default;
    int64_t now_server_ns() const noexcept;
    void sync(int64_t server_ns, int64_t mono_ns, int64_t offset_ns,
              int64_t rtt_ns, int64_t jitter_ns, uint32_t samples) noexcept;
    void set_status(uint32_t status) noexcept;
    ClockSnapshot snapshot() const noexcept;
private:
    ClockState state_;
};
} // namespace clockcore

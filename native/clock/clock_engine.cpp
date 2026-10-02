#include "clock_engine.h"
#include <ctime>
namespace clockcore {
namespace { int64_t ts_ns(const timespec& ts) noexcept { return static_cast<int64_t>(ts.tv_sec) * 1'000'000'000LL + static_cast<int64_t>(ts.tv_nsec); } }
int64_t monotonic_ns() noexcept { timespec ts{}; return ::clock_gettime(CLOCK_MONOTONIC, &ts) == 0 ? ts_ns(ts) : 0; }
int64_t realtime_ns() noexcept { timespec ts{}; return ::clock_gettime(CLOCK_REALTIME, &ts) == 0 ? ts_ns(ts) : 0; }
void ClockState::publish(const ClockSnapshot& s) noexcept {
    offset_ns_.store(s.offset_ns, std::memory_order_relaxed);
    sync_mono_ns_.store(s.sync_mono_ns, std::memory_order_relaxed);
    sync_server_ns_.store(s.sync_server_ns, std::memory_order_relaxed);
    rtt_ns_.store(s.rtt_ns, std::memory_order_relaxed);
    jitter_ns_.store(s.jitter_ns, std::memory_order_relaxed);
    samples_.store(s.samples, std::memory_order_relaxed);
    status_.store(s.status, std::memory_order_release);
}
ClockSnapshot ClockState::snapshot() const noexcept {
    const uint32_t status = status_.load(std::memory_order_acquire);
    ClockSnapshot s{};
    s.offset_ns = offset_ns_.load(std::memory_order_relaxed);
    s.sync_mono_ns = sync_mono_ns_.load(std::memory_order_relaxed);
    s.sync_server_ns = sync_server_ns_.load(std::memory_order_relaxed);
    s.rtt_ns = rtt_ns_.load(std::memory_order_relaxed);
    s.jitter_ns = jitter_ns_.load(std::memory_order_relaxed);
    s.samples = samples_.load(std::memory_order_relaxed);
    s.status = status;
    return s;
}
int64_t ClockEngine::now_server_ns() const noexcept {
    const auto s = state_.snapshot();
    const int64_t mono = monotonic_ns();
    if (s.sync_mono_ns <= 0 || mono <= 0) return 0;
    return s.sync_server_ns + (mono - s.sync_mono_ns);
}
void ClockEngine::sync(int64_t server_ns, int64_t mono_ns, int64_t offset_ns,
                       int64_t rtt_ns, int64_t jitter_ns, uint32_t samples) noexcept {
    ClockSnapshot s{};
    s.offset_ns = offset_ns; s.sync_mono_ns = mono_ns; s.sync_server_ns = server_ns;
    s.rtt_ns = rtt_ns; s.jitter_ns = jitter_ns; s.samples = samples; s.status = 2;
    state_.publish(s);
}
void ClockEngine::set_status(uint32_t status) noexcept { state_.set_status(status); }
ClockSnapshot ClockEngine::snapshot() const noexcept { return state_.snapshot(); }
} // namespace clockcore

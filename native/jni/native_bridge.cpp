#include <jni.h>
#include "../clock/clock_engine.h"
#include "../ntp/ntp_client.h"
#include "../filter/ntp_filter.h"
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
namespace {
constexpr std::uint32_t kUnsynced = 0;
constexpr std::uint32_t kSyncing = 1;
constexpr std::uint32_t kDegraded = 3;
clockcore::ClockEngine g_engine;
std::atomic<bool> g_running{false};
std::thread g_worker;
std::mutex g_lifecycle_mutex;
std::mutex g_wait_mutex;
std::condition_variable g_wait_cv;
std::atomic<bool> g_resync_requested{false};
const std::array<const char*, 5> kServers = {"time.cloudflare.com", "time.google.com", "pool.ntp.org", "time.windows.com", "time.apple.com"};
void worker() {
    ntp::Client client;
    while (g_running.load(std::memory_order_acquire)) {
        g_engine.set_status(kSyncing);
        ntp::Filter filter;
        for (const char* server : kServers) {
            if (!g_running.load(std::memory_order_relaxed)) break;
            ntp::SyncSample sample{};
            if (client.sync_once(server, 123, sample)) filter.add(sample);
        }
        if (filter.size() > 0U) {
            ntp::SyncSample best{}; std::int64_t jitter = 0;
            if (filter.choose(best, jitter)) g_engine.sync(best.corrected_server_ns, best.rx_mono_ns, best.offset_ns, best.delay_ns, jitter, static_cast<std::uint32_t>(filter.size()));
        } else {
            const auto s = g_engine.snapshot();
            g_engine.set_status(s.sync_mono_ns > 0 ? kDegraded : kUnsynced);
        }
        std::unique_lock<std::mutex> lock(g_wait_mutex);
        g_wait_cv.wait_for(lock, std::chrono::seconds(30), [] { return !g_running.load(std::memory_order_relaxed) || g_resync_requested.exchange(false); });
    }
}
}
extern "C" JNIEXPORT void JNICALL Java_com_example_precisionclock_NativeBridge_start(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(g_lifecycle_mutex);
    if (g_running.exchange(true)) return;
    g_worker = std::thread(worker);
}
extern "C" JNIEXPORT void JNICALL Java_com_example_precisionclock_NativeBridge_stop(JNIEnv*, jobject) {
    std::thread worker_to_join;
    { std::lock_guard<std::mutex> lock(g_lifecycle_mutex); if (!g_running.exchange(false)) return; worker_to_join = std::move(g_worker); }
    g_wait_cv.notify_all();
    if (worker_to_join.joinable()) worker_to_join.join();
}
extern "C" JNIEXPORT void JNICALL Java_com_example_precisionclock_NativeBridge_requestSync(JNIEnv*, jobject) { g_resync_requested.store(true, std::memory_order_release); g_wait_cv.notify_all(); }
extern "C" JNIEXPORT jlong JNICALL Java_com_example_precisionclock_NativeBridge_nowServerNs(JNIEnv*, jobject) { return static_cast<jlong>(g_engine.now_server_ns()); }
extern "C" JNIEXPORT jlong JNICALL Java_com_example_precisionclock_NativeBridge_offsetNs(JNIEnv*, jobject) { return static_cast<jlong>(g_engine.snapshot().offset_ns); }
extern "C" JNIEXPORT jlong JNICALL Java_com_example_precisionclock_NativeBridge_rttNs(JNIEnv*, jobject) { return static_cast<jlong>(g_engine.snapshot().rtt_ns); }
extern "C" JNIEXPORT jlong JNICALL Java_com_example_precisionclock_NativeBridge_jitterNs(JNIEnv*, jobject) { return static_cast<jlong>(g_engine.snapshot().jitter_ns); }
extern "C" JNIEXPORT jint JNICALL Java_com_example_precisionclock_NativeBridge_sampleCount(JNIEnv*, jobject) { return static_cast<jint>(g_engine.snapshot().samples); }
extern "C" JNIEXPORT jlong JNICALL Java_com_example_precisionclock_NativeBridge_lastSyncAgeMs(JNIEnv*, jobject) { const auto s = g_engine.snapshot(); const auto now = clockcore::monotonic_ns(); if (s.sync_mono_ns <= 0 || now < s.sync_mono_ns) return -1; return static_cast<jlong>((now - s.sync_mono_ns) / 1'000'000LL); }
extern "C" JNIEXPORT jint JNICALL Java_com_example_precisionclock_NativeBridge_syncStatus(JNIEnv*, jobject) { return static_cast<jint>(g_engine.snapshot().status); }

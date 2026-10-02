#include "ntp_filter.h"
#include <algorithm>
#include <cstdlib>
namespace ntp {
void Filter::add(const SyncSample& s) { samples_.push_back(s); if (samples_.size() > 32U) samples_.erase(samples_.begin()); }
bool Filter::choose(SyncSample& best, std::int64_t& jitter_ns) const noexcept {
    if (samples_.empty()) return false;
    std::vector<std::int64_t> offsets; offsets.reserve(samples_.size());
    for (const auto& s : samples_) offsets.push_back(s.offset_ns);
    std::sort(offsets.begin(), offsets.end());
    const std::int64_t median = offsets[offsets.size() / 2U];
    std::vector<std::int64_t> deviations; deviations.reserve(offsets.size());
    for (const auto v : offsets) deviations.push_back(std::llabs(v - median));
    std::sort(deviations.begin(), deviations.end());
    const std::int64_t mad = deviations[deviations.size() / 2U];
    const std::int64_t threshold = std::max<std::int64_t>(50'000'000LL, mad * 6LL);
    bool found = false;
    for (const auto& s : samples_) {
        if (std::llabs(s.offset_ns - median) <= threshold && (!found || s.delay_ns < best.delay_ns)) { best = s; found = true; }
    }
    if (!found) { best = *std::min_element(samples_.begin(), samples_.end(), [](const auto& a, const auto& b){ return a.delay_ns < b.delay_ns; }); }
    std::int64_t sum = 0; std::size_t count = 0;
    for (const auto& s : samples_) if (std::llabs(s.offset_ns - median) <= threshold) { sum += std::llabs(s.offset_ns - median); ++count; }
    jitter_ns = count == 0U ? 0 : sum / static_cast<std::int64_t>(count);
    return true;
}
} // namespace ntp

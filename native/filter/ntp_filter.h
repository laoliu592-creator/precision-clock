#pragma once
#include "../ntp/ntp_client.h"
#include <cstdint>
#include <vector>
namespace ntp {
class Filter {
public:
    void add(const SyncSample& s);
    bool choose(SyncSample& best, std::int64_t& jitter_ns) const noexcept;
    std::size_t size() const noexcept { return samples_.size(); }
    void clear() noexcept { samples_.clear(); }
private:
    std::vector<SyncSample> samples_;
};
}

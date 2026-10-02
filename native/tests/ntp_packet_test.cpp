#include "../ntp/ntp_packet.h"
#include <cassert>
int main() {
    const auto x = ntp::unix_ns_to_ntp64(1760000000123456789LL);
    const auto y = ntp::ntp64_to_unix_ns(x);
    assert(y > 0);
    assert((y - 1760000000123456789LL) < 1000LL);
    return 0;
}

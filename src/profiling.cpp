#include "cgp/profiling.hpp"

#include <sys/resource.h>

namespace cgp {

double peak_rss_mb() {
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) return 0.0;
#if defined(__APPLE__)
    return static_cast<double>(usage.ru_maxrss) / (1024.0 * 1024.0);
#else
    return static_cast<double>(usage.ru_maxrss) / 1024.0;
#endif
}

}  // namespace cgp

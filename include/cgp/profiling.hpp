#pragma once

#include <chrono>

namespace cgp {

class Stopwatch {
public:
    Stopwatch() : start_(std::chrono::steady_clock::now()) {}
    double seconds() const {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - start_).count();
    }

private:
    std::chrono::steady_clock::time_point start_;
};

// Peak resident set size of the current process in MiB (getrusage ru_maxrss;
// reported in KiB on Linux and in bytes on macOS).
double peak_rss_mb();

}  // namespace cgp

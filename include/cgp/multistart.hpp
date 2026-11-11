#pragma once

#include <cstdint>
#include <vector>

#include "cgp/partition.hpp"
#include "cgp/partitioner.hpp"

namespace cgp {

struct MultiStartOptions {
    int starts = 8;        // number of random initial partitions
    int threads = 0;       // worker threads; 0 = std::thread::hardware_concurrency()
    std::uint64_t seed = 1;
};

struct MultiStartResult {
    Partition best;
    std::int64_t best_cost = 0;
    int best_start = -1;
    std::vector<std::int64_t> costs;  // final cost of every start, by start index
};

// Refines `starts` random balanced bisections in parallel and keeps the one
// with the lowest cost. Start i is seeded from (seed, i) and ties go to the
// lowest start index, so the result does not depend on thread count or
// scheduling.
MultiStartResult run_multistart(const Partitioner& algo, const MultiStartOptions& opts);

}  // namespace cgp

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "cgp/hypergraph.hpp"
#include "cgp/multistart.hpp"

namespace cgp {

enum class Algorithm { KL, FM };

struct RunConfig {
    Algorithm algorithm = Algorithm::FM;
    MultiStartOptions multistart;
    std::size_t stall_limit = 0;      // early-exit move limit, 0 = off
    double balance_eps = 0.02;        // FM only (KL keeps exact balance)
    std::size_t max_net_size = 16;    // KL clique expansion cap
};

struct RunResult {
    MultiStartResult ms;
    std::int64_t net_cut = 0;         // cut nets of the best partition (common metric)
    std::int64_t objective = 0;       // the algorithm's own cost (edge cut for KL)
    double seconds = 0.0;             // model build (KL clique expansion) + multi-start
};

// Builds the model the algorithm needs from the netlist and runs multi-start
// refinement, timing everything after the netlist has been read.
RunResult run_partitioner(const Hypergraph& hg, const RunConfig& cfg);

const char* algorithm_name(Algorithm a);

}  // namespace cgp

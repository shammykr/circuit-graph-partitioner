#pragma once

#include <cstddef>

#include "cgp/graph.hpp"
#include "cgp/partitioner.hpp"

namespace cgp {

struct KLOptions {
    int max_passes = 50;
    // Early exit: abort a pass after this many consecutive swaps that fail to
    // improve the best prefix seen so far in the pass. 0 = run full passes
    // (textbook KL, min(|A|,|B|) swaps per pass).
    std::size_t stall_limit = 0;
};

// Kernighan-Lin bisection on a weighted graph. Each pass swaps pairs (a, b),
// a in side 0 and b in side 1, so block sizes never change: a perfectly
// balanced input stays perfectly balanced.
class KernighanLin final : public Partitioner {
public:
    KernighanLin(const Graph& g, KLOptions opts = {}) : g_(g), opts_(opts) {}

    std::string name() const override { return "KL"; }
    std::size_t num_nodes() const override { return g_.num_nodes(); }
    std::int64_t cost(const Partition& p) const override { return g_.cut(p); }
    RefineStats refine(Partition& p) const override;

private:
    const Graph& g_;
    KLOptions opts_;
};

}  // namespace cgp

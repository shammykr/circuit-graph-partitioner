#pragma once

#include <cstddef>

#include "cgp/hypergraph.hpp"
#include "cgp/partitioner.hpp"

namespace cgp {

struct FMOptions {
    int max_passes = 50;
    // Allowed imbalance: each side may hold up to (1 + eps) * n / 2 nodes.
    double balance_eps = 0.02;
    // Early exit: abort a pass after this many consecutive moves that fail to
    // improve the best prefix seen so far in the pass. 0 = move every node.
    std::size_t stall_limit = 0;
};

// Fiduccia-Mattheyses refinement on the netlist hypergraph, minimising the
// number of cut nets. Single-node moves are chosen from per-side gain buckets
// and gains are updated incrementally from per-net pin counts.
class FiducciaMattheyses final : public Partitioner {
public:
    FiducciaMattheyses(const Hypergraph& hg, FMOptions opts = {}) : hg_(hg), opts_(opts) {}

    std::string name() const override { return "FM"; }
    std::size_t num_nodes() const override { return hg_.num_nodes(); }
    std::int64_t cost(const Partition& p) const override { return hg_.cut(p); }
    RefineStats refine(Partition& p) const override;

private:
    const Hypergraph& hg_;
    FMOptions opts_;
};

}  // namespace cgp

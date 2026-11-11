#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "cgp/types.hpp"

namespace cgp {

class Hypergraph;
class Partition;

struct Edge {
    NodeId u;
    NodeId v;
    Weight w;
};

// Undirected weighted graph in compressed sparse row (CSR) form.
//
// Neighbours of node v live in targets_[offsets_[v] .. offsets_[v+1]) and the
// matching edge weights in weights_ at the same indices (structure-of-arrays).
// Every traversal in the partitioners is a linear scan over these contiguous
// ranges, so there is no pointer chasing and no per-node allocation.
class Graph {
public:
    Graph() = default;
    // Self loops are dropped; parallel edges are merged by summing weights.
    Graph(std::size_t num_nodes, const std::vector<Edge>& edges);

    std::size_t num_nodes() const { return offsets_.empty() ? 0 : offsets_.size() - 1; }
    std::size_t num_edges() const { return targets_.size() / 2; }

    std::uint32_t begin(NodeId v) const { return offsets_[v]; }
    std::uint32_t end(NodeId v) const { return offsets_[v + 1]; }
    NodeId target(std::uint32_t i) const { return targets_[i]; }
    Weight weight(std::uint32_t i) const { return weights_[i]; }

    Weight weighted_degree(NodeId v) const;
    Weight max_weighted_degree() const;

    // Sum of weights of edges whose endpoints are on different sides.
    std::int64_t cut(const Partition& p) const;

private:
    std::vector<std::uint32_t> offsets_;
    std::vector<NodeId> targets_;
    std::vector<Weight> weights_;
};

// Clique expansion of a netlist: every net with 2..max_net_size pins becomes a
// clique of unit-weight edges (parallel edges from different nets add up).
// Nets above the cap are skipped to keep the graph from blowing up.
Graph clique_expand(const Hypergraph& hg, std::size_t max_net_size);

}  // namespace cgp

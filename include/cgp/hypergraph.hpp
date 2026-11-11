#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "cgp/types.hpp"

namespace cgp {

class Partition;

// Netlist hypergraph stored as two CSR incidence arrays:
//   net -> pins : pins_[net_offsets_[e] .. net_offsets_[e+1])
//   node -> nets: node_nets_[node_offsets_[v] .. node_offsets_[v+1])
// FM needs both directions on every move, and both are flat vectors.
class Hypergraph {
public:
    Hypergraph() = default;
    // Duplicate pins inside a net are removed and nets with fewer than two
    // distinct pins are dropped (they can never be cut).
    Hypergraph(std::size_t num_nodes, const std::vector<std::vector<NodeId>>& nets);

    std::size_t num_nodes() const { return num_nodes_; }
    std::size_t num_nets() const { return net_offsets_.empty() ? 0 : net_offsets_.size() - 1; }
    std::size_t num_pins() const { return pins_.size(); }

    std::uint32_t net_begin(NetId e) const { return net_offsets_[e]; }
    std::uint32_t net_end(NetId e) const { return net_offsets_[e + 1]; }
    NodeId pin(std::uint32_t i) const { return pins_[i]; }
    std::size_t net_size(NetId e) const { return net_offsets_[e + 1] - net_offsets_[e]; }

    std::uint32_t node_begin(NodeId v) const { return node_offsets_[v]; }
    std::uint32_t node_end(NodeId v) const { return node_offsets_[v + 1]; }
    NetId incident_net(std::uint32_t i) const { return node_nets_[i]; }
    std::size_t node_degree(NodeId v) const { return node_offsets_[v + 1] - node_offsets_[v]; }
    std::size_t max_node_degree() const;

    // Number of nets with pins on both sides.
    std::int64_t cut(const Partition& p) const;

private:
    std::size_t num_nodes_ = 0;
    std::vector<std::uint32_t> net_offsets_;
    std::vector<NodeId> pins_;
    std::vector<std::uint32_t> node_offsets_;
    std::vector<NetId> node_nets_;
};

}  // namespace cgp

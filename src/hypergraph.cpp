#include "cgp/hypergraph.hpp"

#include <algorithm>
#include <stdexcept>

#include "cgp/partition.hpp"

namespace cgp {

Hypergraph::Hypergraph(std::size_t num_nodes, const std::vector<std::vector<NodeId>>& nets)
    : num_nodes_(num_nodes) {
    net_offsets_.push_back(0);
    std::vector<NodeId> scratch;
    for (const auto& net : nets) {
        scratch = net;
        for (NodeId v : scratch) {
            if (v >= num_nodes) throw std::out_of_range("pin out of range");
        }
        std::sort(scratch.begin(), scratch.end());
        scratch.erase(std::unique(scratch.begin(), scratch.end()), scratch.end());
        if (scratch.size() < 2) continue;
        pins_.insert(pins_.end(), scratch.begin(), scratch.end());
        net_offsets_.push_back(static_cast<std::uint32_t>(pins_.size()));
    }

    // Build the transposed node -> nets incidence with a counting sort.
    node_offsets_.assign(num_nodes + 1, 0);
    for (NodeId v : pins_) ++node_offsets_[v + 1];
    for (std::size_t v = 0; v < num_nodes; ++v) node_offsets_[v + 1] += node_offsets_[v];
    node_nets_.resize(pins_.size());
    std::vector<std::uint32_t> pos(node_offsets_.begin(), node_offsets_.end() - 1);
    for (NetId e = 0; e < num_nets(); ++e) {
        for (std::uint32_t i = net_begin(e); i < net_end(e); ++i) node_nets_[pos[pins_[i]]++] = e;
    }
}

std::size_t Hypergraph::max_node_degree() const {
    std::size_t best = 0;
    for (std::size_t v = 0; v < num_nodes_; ++v) best = std::max(best, node_degree(static_cast<NodeId>(v)));
    return best;
}

std::int64_t Hypergraph::cut(const Partition& p) const {
    std::int64_t total = 0;
    for (NetId e = 0; e < num_nets(); ++e) {
        const auto s0 = p.side(pins_[net_begin(e)]);
        for (std::uint32_t i = net_begin(e) + 1; i < net_end(e); ++i) {
            if (p.side(pins_[i]) != s0) {
                ++total;
                break;
            }
        }
    }
    return total;
}

}  // namespace cgp

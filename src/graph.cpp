#include "cgp/graph.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "cgp/hypergraph.hpp"
#include "cgp/partition.hpp"

namespace cgp {

Graph::Graph(std::size_t num_nodes, const std::vector<Edge>& edges) {
    // Counting sort of directed half-edges by source node.
    std::vector<std::uint32_t> start(num_nodes + 1, 0);
    for (const Edge& e : edges) {
        if (e.u >= num_nodes || e.v >= num_nodes) throw std::out_of_range("edge endpoint out of range");
        if (e.u == e.v) continue;
        ++start[e.u + 1];
        ++start[e.v + 1];
    }
    for (std::size_t i = 0; i < num_nodes; ++i) start[i + 1] += start[i];

    std::vector<std::pair<NodeId, Weight>> half(start[num_nodes]);
    std::vector<std::uint32_t> pos(start.begin(), start.end() - 1);
    for (const Edge& e : edges) {
        if (e.u == e.v) continue;
        half[pos[e.u]++] = {e.v, e.w};
        half[pos[e.v]++] = {e.u, e.w};
    }

    // Sort each adjacency range and merge parallel edges into the final CSR.
    offsets_.assign(num_nodes + 1, 0);
    targets_.reserve(half.size());
    weights_.reserve(half.size());
    for (std::size_t v = 0; v < num_nodes; ++v) {
        auto first = half.begin() + start[v];
        auto last = half.begin() + start[v + 1];
        std::sort(first, last, [](const auto& a, const auto& b) { return a.first < b.first; });
        for (auto it = first; it != last; ++it) {
            if (targets_.size() > offsets_[v] && targets_.back() == it->first) {
                weights_.back() += it->second;
            } else {
                targets_.push_back(it->first);
                weights_.push_back(it->second);
            }
        }
        offsets_[v + 1] = static_cast<std::uint32_t>(targets_.size());
    }
    targets_.shrink_to_fit();
    weights_.shrink_to_fit();
}

Weight Graph::weighted_degree(NodeId v) const {
    Weight d = 0;
    for (std::uint32_t i = begin(v); i < end(v); ++i) d += weights_[i];
    return d;
}

Weight Graph::max_weighted_degree() const {
    Weight best = 0;
    for (std::size_t v = 0; v < num_nodes(); ++v) best = std::max(best, weighted_degree(static_cast<NodeId>(v)));
    return best;
}

std::int64_t Graph::cut(const Partition& p) const {
    std::int64_t total = 0;
    for (std::size_t v = 0; v < num_nodes(); ++v) {
        const auto sv = p.side(static_cast<NodeId>(v));
        for (std::uint32_t i = begin(static_cast<NodeId>(v)); i < end(static_cast<NodeId>(v)); ++i) {
            if (p.side(targets_[i]) != sv) total += weights_[i];
        }
    }
    return total / 2;  // every cut edge was seen from both endpoints
}

Graph clique_expand(const Hypergraph& hg, std::size_t max_net_size) {
    std::vector<Edge> edges;
    for (NetId e = 0; e < hg.num_nets(); ++e) {
        const std::size_t k = hg.net_size(e);
        if (k < 2 || k > max_net_size) continue;
        for (std::uint32_t i = hg.net_begin(e); i < hg.net_end(e); ++i) {
            for (std::uint32_t j = i + 1; j < hg.net_end(e); ++j) {
                edges.push_back({hg.pin(i), hg.pin(j), 1});
            }
        }
    }
    return Graph(hg.num_nodes(), edges);
}

}  // namespace cgp

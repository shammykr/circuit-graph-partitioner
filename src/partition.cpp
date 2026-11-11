#include "cgp/partition.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace cgp {

std::size_t max_side_size(std::size_t n, double eps) {
    // As in the FM paper, always allow at least one cell of slack beyond a
    // perfect split; otherwise single-node moves are impossible from a
    // perfectly balanced start.
    const std::size_t one_cell = n / 2 + 1;
    const auto with_eps = static_cast<std::size_t>(std::floor((1.0 + eps) * static_cast<double>(n) / 2.0));
    return std::min(n, std::max(one_cell, with_eps));
}

Partition::Partition(std::size_t num_nodes) : side_(num_nodes, 0), size_{num_nodes, 0} {}

Partition Partition::random_balanced(std::size_t num_nodes, std::mt19937_64& rng) {
    std::vector<NodeId> order(num_nodes);
    std::iota(order.begin(), order.end(), NodeId{0});
    std::shuffle(order.begin(), order.end(), rng);
    Partition p(num_nodes);
    for (std::size_t i = num_nodes / 2; i < num_nodes; ++i) p.assign(order[i], 1);
    return p;
}

void Partition::assign(NodeId v, std::uint8_t s) {
    if (side_[v] == s) return;
    --size_[side_[v]];
    ++size_[s];
    side_[v] = s;
}

void Partition::move(NodeId v) { assign(v, static_cast<std::uint8_t>(side_[v] ^ 1u)); }

bool Partition::is_balanced(double eps) const {
    return std::max(size_[0], size_[1]) <= max_side_size(num_nodes(), eps);
}

}  // namespace cgp

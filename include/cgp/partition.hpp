#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include "cgp/types.hpp"

namespace cgp {

// Largest block size allowed for a bisection of `n` unit-weight nodes with
// imbalance tolerance `eps` (0.02 = each side may hold up to 51% of nodes).
std::size_t max_side_size(std::size_t n, double eps);

// A two-way partition: one byte per node holding its side (0 or 1), plus the
// running size of each side so balance checks are O(1).
class Partition {
public:
    Partition() = default;
    explicit Partition(std::size_t num_nodes);

    // Random perfectly balanced bisection (floor(n/2) nodes on side 0).
    static Partition random_balanced(std::size_t num_nodes, std::mt19937_64& rng);

    std::size_t num_nodes() const { return side_.size(); }
    std::uint8_t side(NodeId v) const { return side_[v]; }
    std::size_t size(int s) const { return size_[s]; }
    const std::vector<std::uint8_t>& sides() const { return side_; }

    void assign(NodeId v, std::uint8_t s);
    void move(NodeId v);  // flip v to the other side

    bool is_balanced(double eps) const;

private:
    std::vector<std::uint8_t> side_;
    std::array<std::size_t, 2> size_{0, 0};
};

}  // namespace cgp

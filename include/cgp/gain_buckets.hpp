#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "cgp/types.hpp"

namespace cgp {

// Bucket priority queue for integer gains in [-max_gain, +max_gain], the
// classic Fiduccia-Mattheyses structure.
//
// Bucket g holds an intrusive doubly linked list of nodes with gain g. The
// links (next_/prev_) are flat per-node arrays rather than heap-allocated list
// nodes, so insert, remove and gain update are all O(1). top_ caches the index
// of the highest possibly non-empty bucket and is lowered lazily on lookup.
class GainBuckets {
public:
    static constexpr std::int32_t kNone = -1;

    GainBuckets(std::size_t num_nodes, std::int32_t max_gain)
        : max_gain_(max_gain),
          heads_(2 * static_cast<std::size_t>(max_gain) + 1, kNone),
          next_(num_nodes, kNone),
          prev_(num_nodes, kNone),
          gain_(num_nodes, 0),
          in_(num_nodes, 0) {}

    void clear() {
        std::fill(heads_.begin(), heads_.end(), kNone);
        std::fill(in_.begin(), in_.end(), 0);
        top_ = -1;
        count_ = 0;
    }

    bool empty() const { return count_ == 0; }
    bool contains(NodeId v) const { return in_[v] != 0; }
    std::int32_t gain(NodeId v) const { return gain_[v]; }

    void insert(NodeId v, std::int32_t g) {
        assert(!in_[v] && g >= -max_gain_ && g <= max_gain_);
        const std::int32_t idx = g + max_gain_;
        const std::int32_t head = heads_[idx];
        prev_[v] = kNone;
        next_[v] = head;
        if (head != kNone) prev_[head] = static_cast<std::int32_t>(v);
        heads_[idx] = static_cast<std::int32_t>(v);
        gain_[v] = g;
        in_[v] = 1;
        top_ = std::max(top_, idx);
        ++count_;
    }

    void remove(NodeId v) {
        assert(in_[v]);
        if (prev_[v] != kNone) {
            next_[prev_[v]] = next_[v];
        } else {
            heads_[gain_[v] + max_gain_] = next_[v];
        }
        if (next_[v] != kNone) prev_[next_[v]] = prev_[v];
        in_[v] = 0;
        --count_;
    }

    void adjust(NodeId v, std::int32_t delta) {
        const std::int32_t g = gain_[v] + delta;
        remove(v);
        insert(v, g);
    }

    // Node with the highest gain, or kNone if empty.
    std::int32_t top() const {
        while (top_ >= 0 && heads_[top_] == kNone) --top_;
        return top_ < 0 ? kNone : heads_[top_];
    }

    // Iteration in non-increasing gain order: for (v = top(); v != kNone; v = next_desc(v)).
    std::int32_t next_desc(NodeId v) const {
        if (next_[v] != kNone) return next_[v];
        for (std::int32_t idx = gain_[v] + max_gain_ - 1; idx >= 0; --idx) {
            if (heads_[idx] != kNone) return heads_[idx];
        }
        return kNone;
    }

private:
    std::int32_t max_gain_;
    std::vector<std::int32_t> heads_;
    std::vector<std::int32_t> next_;
    std::vector<std::int32_t> prev_;
    std::vector<std::int32_t> gain_;
    std::vector<std::uint8_t> in_;
    mutable std::int32_t top_ = -1;
    std::size_t count_ = 0;
};

}  // namespace cgp

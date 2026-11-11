#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "cgp/partition.hpp"

namespace cgp {

struct RefineStats {
    int passes = 0;            // improvement passes executed
    std::int64_t moves = 0;    // node moves kept after rollback
};

// Common interface for iterative-improvement bisection algorithms.
//
// refine() is const and keeps all scratch state on its own stack, so a single
// Partitioner instance can be shared by many threads (see multistart.hpp).
class Partitioner {
public:
    virtual ~Partitioner() = default;

    virtual std::string name() const = 0;
    virtual std::size_t num_nodes() const = 0;

    // Objective the algorithm minimises (edge cut for KL, net cut for FM).
    virtual std::int64_t cost(const Partition& p) const = 0;

    // Improve `p` in place. `p` must already satisfy the algorithm's balance
    // constraint; the result does too.
    virtual RefineStats refine(Partition& p) const = 0;
};

}  // namespace cgp

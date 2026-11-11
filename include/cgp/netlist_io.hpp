#pragma once

#include <string>

#include "cgp/hypergraph.hpp"
#include "cgp/partition.hpp"

namespace cgp {

// Reads an hMETIS-style .hgr netlist:
//   % comment lines are ignored
//   <num_nets> <num_nodes>
//   one line per net listing its pins as 1-based node ids
// Weighted formats (a third header field other than 0) are rejected.
Hypergraph read_hgr(const std::string& path);

void write_hgr(const std::string& path, const Hypergraph& hg);

// One line per node with its side (0/1), the hMETIS partition file format.
void write_partition(const std::string& path, const Partition& p);

}  // namespace cgp

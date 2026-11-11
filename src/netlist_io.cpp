#include "cgp/netlist_io.hpp"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace cgp {

namespace {

bool next_data_line(std::istream& in, std::string& line) {
    while (std::getline(in, line)) {
        const auto first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '%') continue;
        return true;
    }
    return false;
}

}  // namespace

Hypergraph read_hgr(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open " + path);

    std::string line;
    if (!next_data_line(in, line)) throw std::runtime_error(path + ": missing header");
    std::istringstream header(line);
    std::size_t num_nets = 0, num_nodes = 0;
    int fmt = 0;
    if (!(header >> num_nets >> num_nodes)) throw std::runtime_error(path + ": bad header");
    if (header >> fmt && fmt != 0) throw std::runtime_error(path + ": weighted .hgr formats are not supported");

    std::vector<std::vector<NodeId>> nets(num_nets);
    for (std::size_t e = 0; e < num_nets; ++e) {
        if (!next_data_line(in, line)) throw std::runtime_error(path + ": expected " + std::to_string(num_nets) + " nets");
        const char* p = line.c_str();
        char* endp = nullptr;
        for (;;) {
            const unsigned long id = std::strtoul(p, &endp, 10);
            if (endp == p) break;
            if (id == 0 || id > num_nodes) throw std::runtime_error(path + ": pin id out of range on net " + std::to_string(e + 1));
            nets[e].push_back(static_cast<NodeId>(id - 1));
            p = endp;
        }
    }
    return Hypergraph(num_nodes, nets);
}

void write_hgr(const std::string& path, const Hypergraph& hg) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot write " + path);
    out << hg.num_nets() << ' ' << hg.num_nodes() << '\n';
    for (NetId e = 0; e < hg.num_nets(); ++e) {
        for (std::uint32_t i = hg.net_begin(e); i < hg.net_end(e); ++i) {
            out << (i == hg.net_begin(e) ? "" : " ") << hg.pin(i) + 1;
        }
        out << '\n';
    }
}

void write_partition(const std::string& path, const Partition& p) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("cannot write " + path);
    for (std::uint8_t s : p.sides()) out << static_cast<int>(s) << '\n';
}

}  // namespace cgp

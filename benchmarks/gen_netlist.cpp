// cgp_gen: synthetic circuit-like netlist generator.
//
// Cells sit on a sqrt(N) x sqrt(N) grid (row-major ids). Every cell drives one
// net whose sinks are mostly nearby cells (within +/-2 grid steps), with a
// small fraction of global sinks. Fanout follows a skewed distribution
// (mostly 1-3 sinks). This gives the locality real placed netlists have, so a
// good bisection cuts far fewer nets than a random one.
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "cgp/hypergraph.hpp"
#include "cgp/netlist_io.hpp"

int main(int argc, char** argv) {
    std::size_t n = 0;
    std::uint64_t seed = 1;
    double global_p = 0.03;
    std::string out;
    for (int i = 1; i + 1 < argc; i += 2) {
        const std::string a = argv[i];
        if (a == "--nodes") n = std::stoull(argv[i + 1]);
        else if (a == "--seed") seed = std::stoull(argv[i + 1]);
        else if (a == "--global") global_p = std::stod(argv[i + 1]);
        else if (a == "--out") out = argv[i + 1];
    }
    if (n < 2 || out.empty()) {
        std::cerr << "usage: cgp_gen --nodes N --out FILE [--seed S] [--global P]\n";
        return 2;
    }

    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::uniform_int_distribution<int> step(-2, 2);
    std::uniform_int_distribution<std::size_t> any(0, n - 1);
    const auto width = static_cast<long>(std::ceil(std::sqrt(static_cast<double>(n))));
    const long rows = static_cast<long>((n + width - 1) / width);

    std::vector<std::vector<cgp::NodeId>> nets(n);
    for (std::size_t v = 0; v < n; ++v) {
        const double r = unit(rng);
        const int fanout = r < 0.45 ? 1 : r < 0.70 ? 2 : r < 0.85 ? 3 : 4 + static_cast<int>(rng() % 5);
        const long x = static_cast<long>(v) % width, y = static_cast<long>(v) / width;
        nets[v].push_back(static_cast<cgp::NodeId>(v));
        for (int k = 0; k < fanout; ++k) {
            std::size_t u;
            if (unit(rng) < global_p) {
                u = any(rng);
            } else {
                const long ux = std::clamp(x + step(rng), 0L, width - 1);
                const long uy = std::clamp(y + step(rng), 0L, rows - 1);
                u = std::min<std::size_t>(static_cast<std::size_t>(uy * width + ux), n - 1);
            }
            nets[v].push_back(static_cast<cgp::NodeId>(u));
        }
    }
    const cgp::Hypergraph hg(n, nets);  // drops duplicate pins and 1-pin nets
    cgp::write_hgr(out, hg);
    std::cout << out << ": " << hg.num_nodes() << " nodes, " << hg.num_nets() << " nets, " << hg.num_pins()
              << " pins\n";
    return 0;
}

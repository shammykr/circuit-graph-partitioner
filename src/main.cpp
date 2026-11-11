// cgp: partition one netlist and report cut size, runtime and peak memory.
#include <algorithm>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <thread>

#include "cgp/driver.hpp"
#include "cgp/netlist_io.hpp"
#include "cgp/profiling.hpp"

namespace {

void usage() {
    std::cerr << "usage: cgp <netlist.hgr> [options]\n"
                 "  --algo kl|fm        algorithm (default fm)\n"
                 "  --starts N          random initial partitions (default 8)\n"
                 "  --threads N         worker threads (default: hardware concurrency)\n"
                 "  --seed N            RNG seed (default 1)\n"
                 "  --eps X             FM balance tolerance (default 0.02)\n"
                 "  --stall-limit N     early-exit after N non-improving moves per pass (default 0 = off)\n"
                 "  --max-net N         KL: skip nets larger than N in clique expansion (default 16)\n"
                 "  --out FILE          write the partition (one side per line)\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        usage();
        return 2;
    }
    std::string path, out_path;
    cgp::RunConfig cfg;
    try {
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            auto value = [&]() -> std::string {
                if (i + 1 >= argc) throw std::invalid_argument("missing value for " + arg);
                return argv[++i];
            };
            if (arg == "--algo") {
                const std::string a = value();
                if (a == "kl") cfg.algorithm = cgp::Algorithm::KL;
                else if (a == "fm") cfg.algorithm = cgp::Algorithm::FM;
                else throw std::invalid_argument("unknown algorithm " + a);
            } else if (arg == "--starts") cfg.multistart.starts = std::stoi(value());
            else if (arg == "--threads") cfg.multistart.threads = std::stoi(value());
            else if (arg == "--seed") cfg.multistart.seed = std::stoull(value());
            else if (arg == "--eps") cfg.balance_eps = std::stod(value());
            else if (arg == "--stall-limit") cfg.stall_limit = std::stoull(value());
            else if (arg == "--max-net") cfg.max_net_size = std::stoull(value());
            else if (arg == "--out") out_path = value();
            else if (arg == "-h" || arg == "--help") { usage(); return 0; }
            else if (!arg.empty() && arg[0] == '-') throw std::invalid_argument("unknown option " + arg);
            else path = arg;
        }
        if (path.empty()) throw std::invalid_argument("no netlist given");

        const cgp::Hypergraph hg = cgp::read_hgr(path);
        const cgp::RunResult r = cgp::run_partitioner(hg, cfg);
        const auto& best = r.ms.best;
        const int threads = cfg.multistart.threads > 0 ? cfg.multistart.threads
                                                       : static_cast<int>(std::thread::hardware_concurrency());

        std::cout << "netlist     : " << path << " (" << hg.num_nodes() << " nodes, " << hg.num_nets()
                  << " nets, " << hg.num_pins() << " pins)\n"
                  << "algorithm   : " << cgp::algorithm_name(cfg.algorithm) << ", " << cfg.multistart.starts
                  << " starts on " << std::min(threads, cfg.multistart.starts) << " threads, stall limit "
                  << cfg.stall_limit << "\n"
                  << "net cut     : " << r.net_cut << "\n";
        if (cfg.algorithm == cgp::Algorithm::KL) std::cout << "edge cut    : " << r.objective << " (clique graph)\n";
        const auto [lo, hi] = std::minmax_element(r.ms.costs.begin(), r.ms.costs.end());
        std::cout << "start costs : min " << *lo << ", max " << *hi << " (best = start " << r.ms.best_start << ")\n"
                  << "block sizes : " << best.size(0) << " / " << best.size(1) << "\n"
                  << "runtime     : " << r.seconds << " s\n"
                  << "peak RSS    : " << cgp::peak_rss_mb() << " MB\n";

        if (!out_path.empty()) cgp::write_partition(out_path, best);
    } catch (const std::exception& e) {
        std::cerr << "cgp: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

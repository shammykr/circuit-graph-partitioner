// cgp_bench: profiling harness. For every netlist and algorithm it forks a
// child process that reads the netlist, partitions it and reports net cut,
// wall-clock runtime and peak RSS back over a pipe. A fresh process per run
// keeps the peak-RSS figure specific to that run.
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <random>
#include <string>
#include <vector>

#include "cgp/driver.hpp"
#include "cgp/netlist_io.hpp"
#include "cgp/profiling.hpp"

namespace {

struct Report {
    bool ok = false;
    std::size_t nodes = 0, nets = 0, pins = 0;
    std::int64_t random_cut = 0, net_cut = 0, objective = 0;
    std::size_t size0 = 0, size1 = 0;
    double seconds = 0.0, rss_mb = 0.0;
};

Report run_child(const std::string& path, const cgp::RunConfig& cfg) {
    Report r;
    const cgp::Hypergraph hg = cgp::read_hgr(path);
    r.nodes = hg.num_nodes();
    r.nets = hg.num_nets();
    r.pins = hg.num_pins();
    std::mt19937_64 rng(cfg.multistart.seed);
    r.random_cut = hg.cut(cgp::Partition::random_balanced(hg.num_nodes(), rng));
    const cgp::RunResult res = cgp::run_partitioner(hg, cfg);
    r.net_cut = res.net_cut;
    r.objective = res.objective;
    r.size0 = res.ms.best.size(0);
    r.size1 = res.ms.best.size(1);
    r.seconds = res.seconds;
    r.rss_mb = cgp::peak_rss_mb();
    r.ok = true;
    return r;
}

Report run_isolated(const std::string& path, const cgp::RunConfig& cfg) {
    int fds[2];
    if (pipe(fds) != 0) return {};
    const pid_t pid = fork();
    if (pid == 0) {
        close(fds[0]);
        Report r;
        try {
            r = run_child(path, cfg);
        } catch (const std::exception& e) {
            std::cerr << path << ": " << e.what() << "\n";
        }
        ssize_t written = write(fds[1], &r, sizeof r);
        _exit(written == static_cast<ssize_t>(sizeof r) ? 0 : 1);
    }
    close(fds[1]);
    Report r;
    if (read(fds[0], &r, sizeof r) != static_cast<ssize_t>(sizeof r)) r.ok = false;
    close(fds[0]);
    int status = 0;
    waitpid(pid, &status, 0);
    return r;
}

}  // namespace

int main(int argc, char** argv) {
    cgp::RunConfig cfg;
    std::vector<std::string> files;
    try {
        for (int i = 1; i < argc; ++i) {
            const std::string a = argv[i];
            auto value = [&] {
                if (i + 1 >= argc) throw std::invalid_argument("missing value for " + a);
                return std::string(argv[++i]);
            };
            if (a == "--starts") cfg.multistart.starts = std::stoi(value());
            else if (a == "--threads") cfg.multistart.threads = std::stoi(value());
            else if (a == "--seed") cfg.multistart.seed = std::stoull(value());
            else if (a == "--stall-limit") cfg.stall_limit = std::stoull(value());
            else if (a == "--eps") cfg.balance_eps = std::stod(value());
            else if (!a.empty() && a[0] == '-') throw std::invalid_argument("unknown option " + a);
            else files.push_back(a);
        }
    } catch (const std::exception& e) {
        std::cerr << "cgp_bench: bad arguments: " << e.what() << "\n";
        return 2;
    }
    if (files.empty()) {
        std::cerr << "usage: cgp_bench [--starts N] [--threads N] [--seed N] [--stall-limit N] [--eps X] netlist.hgr...\n";
        return 2;
    }

    std::cout << "starts=" << cfg.multistart.starts << " threads="
              << (cfg.multistart.threads ? std::to_string(cfg.multistart.threads) : std::string("auto"))
              << " seed=" << cfg.multistart.seed << " stall_limit=" << cfg.stall_limit << " eps=" << cfg.balance_eps
              << "\n\n"
              << "| Netlist | Nodes | Nets | Pins | Algo | Random net cut | Net cut | Edge cut (KL graph) | Blocks | Runtime (s) | Peak RSS (MB) |\n"
              << "|---|---:|---:|---:|---|---:|---:|---:|---|---:|---:|\n";
    for (const auto& f : files) {
        for (cgp::Algorithm algo : {cgp::Algorithm::KL, cgp::Algorithm::FM}) {
            cfg.algorithm = algo;
            const Report r = run_isolated(f, cfg);
            const std::string name = f.substr(f.find_last_of('/') + 1);
            if (!r.ok) {
                std::cout << "| " << name << " | run failed |\n";
                continue;
            }
            char line[512];
            std::snprintf(line, sizeof line, "| %s | %zu | %zu | %zu | %s | %lld | %lld | %s | %zu / %zu | %.3f | %.1f |\n",
                          name.c_str(), r.nodes, r.nets, r.pins, cgp::algorithm_name(algo),
                          static_cast<long long>(r.random_cut), static_cast<long long>(r.net_cut),
                          algo == cgp::Algorithm::KL ? std::to_string(r.objective).c_str() : "-", r.size0, r.size1,
                          r.seconds, r.rss_mb);
            std::cout << line << std::flush;
        }
    }
    return 0;
}

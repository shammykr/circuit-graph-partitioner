#include "cgp/driver.hpp"

#include "cgp/fiduccia_mattheyses.hpp"
#include "cgp/graph.hpp"
#include "cgp/kernighan_lin.hpp"
#include "cgp/profiling.hpp"

namespace cgp {

const char* algorithm_name(Algorithm a) { return a == Algorithm::KL ? "KL" : "FM"; }

RunResult run_partitioner(const Hypergraph& hg, const RunConfig& cfg) {
    RunResult r;
    Stopwatch clock;
    if (cfg.algorithm == Algorithm::KL) {
        const Graph g = clique_expand(hg, cfg.max_net_size);
        KLOptions opts;
        opts.stall_limit = cfg.stall_limit;
        r.ms = run_multistart(KernighanLin(g, opts), cfg.multistart);
    } else {
        FMOptions opts;
        opts.stall_limit = cfg.stall_limit;
        opts.balance_eps = cfg.balance_eps;
        r.ms = run_multistart(FiducciaMattheyses(hg, opts), cfg.multistart);
    }
    r.seconds = clock.seconds();
    r.objective = r.ms.best_cost;
    r.net_cut = hg.cut(r.ms.best);
    return r;
}

}  // namespace cgp

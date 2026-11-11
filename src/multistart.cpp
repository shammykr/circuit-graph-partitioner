#include "cgp/multistart.hpp"

#include <algorithm>
#include <atomic>
#include <limits>
#include <mutex>
#include <random>
#include <thread>

namespace cgp {

MultiStartResult run_multistart(const Partitioner& algo, const MultiStartOptions& opts) {
    const int starts = std::max(1, opts.starts);
    int threads = opts.threads > 0 ? opts.threads : static_cast<int>(std::thread::hardware_concurrency());
    threads = std::clamp(threads, 1, starts);

    MultiStartResult result;
    result.costs.assign(starts, 0);
    result.best_cost = std::numeric_limits<std::int64_t>::max();

    std::atomic<int> next{0};
    std::mutex best_mutex;

    auto worker = [&] {
        for (int i = next.fetch_add(1); i < starts; i = next.fetch_add(1)) {
            std::seed_seq seq{static_cast<std::uint32_t>(opts.seed), static_cast<std::uint32_t>(opts.seed >> 32),
                              static_cast<std::uint32_t>(i)};
            std::mt19937_64 rng(seq);
            Partition p = Partition::random_balanced(algo.num_nodes(), rng);
            algo.refine(p);
            const std::int64_t c = algo.cost(p);
            result.costs[i] = c;  // each index is written by exactly one thread

            std::lock_guard<std::mutex> lock(best_mutex);
            if (c < result.best_cost || (c == result.best_cost && i < result.best_start)) {
                result.best_cost = c;
                result.best_start = i;
                result.best = std::move(p);
            }
        }
    };

    std::vector<std::thread> pool;
    pool.reserve(threads);
    for (int t = 0; t < threads; ++t) pool.emplace_back(worker);
    for (auto& t : pool) t.join();
    return result;
}

}  // namespace cgp

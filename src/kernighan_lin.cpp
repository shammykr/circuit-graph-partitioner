#include "cgp/kernighan_lin.hpp"

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

#include "cgp/gain_buckets.hpp"

namespace cgp {

RefineStats KernighanLin::refine(Partition& p) const {
    RefineStats stats;
    const std::size_t n = g_.num_nodes();
    if (n < 2) return stats;

    // D(v) = external - internal edge weight is bounded by the weighted
    // degree, so the D values of each side fit in a bucket structure.
    const Weight max_d = std::max<Weight>(g_.max_weighted_degree(), 1);
    GainBuckets side_d[2] = {GainBuckets(n, max_d), GainBuckets(n, max_d)};
    std::vector<Weight> w_to_a(n, 0);  // w(a, x) for the candidate a being scanned
    std::vector<std::pair<NodeId, NodeId>> swaps;
    swaps.reserve(n / 2);

    // Move one node and fix up D of its unlocked neighbours. Applying the two
    // halves of a swap one after the other gives the standard KL update.
    auto move_node = [&](NodeId v) {
        const auto sv = p.side(v);
        for (std::uint32_t i = g_.begin(v); i < g_.end(v); ++i) {
            const NodeId x = g_.target(i);
            const auto sx = p.side(x);
            if (!side_d[sx].contains(x)) continue;  // locked
            const Weight w = g_.weight(i);
            side_d[sx].adjust(x, sx == sv ? 2 * w : -2 * w);
        }
        p.move(v);
    };

    for (int pass = 0; pass < opts_.max_passes; ++pass) {
        ++stats.passes;
        side_d[0].clear();
        side_d[1].clear();
        for (NodeId v = 0; v < n; ++v) {
            Weight d = 0;
            for (std::uint32_t i = g_.begin(v); i < g_.end(v); ++i) {
                d += p.side(g_.target(i)) != p.side(v) ? g_.weight(i) : -g_.weight(i);
            }
            side_d[p.side(v)].insert(v, d);
        }

        const std::size_t max_swaps = std::min(p.size(0), p.size(1));
        std::int64_t cumulative = 0, best = 0;
        std::size_t best_len = 0, stall = 0;
        swaps.clear();

        while (swaps.size() < max_swaps) {
            // Pick the unlocked pair maximising D(a) + D(b) - 2 w(a, b). Both
            // sides are scanned in decreasing D order; because w >= 0,
            // D(a) + D(b) bounds the pair gain and lets us stop early (the
            // pruning from the original KL paper). Usually only a handful of
            // pairs are examined per step.
            std::int64_t best_gain = std::numeric_limits<std::int64_t>::min();
            std::int32_t best_a = GainBuckets::kNone, best_b = GainBuckets::kNone;
            const std::int32_t top_b = side_d[1].top();
            if (side_d[0].top() == GainBuckets::kNone || top_b == GainBuckets::kNone) break;
            const std::int64_t max_db = side_d[1].gain(static_cast<NodeId>(top_b));

            for (std::int32_t a = side_d[0].top(); a != GainBuckets::kNone; a = side_d[0].next_desc(a)) {
                const std::int64_t da = side_d[0].gain(a);
                if (best_a != GainBuckets::kNone && da + max_db <= best_gain) break;
                for (std::uint32_t i = g_.begin(a); i < g_.end(a); ++i) w_to_a[g_.target(i)] = g_.weight(i);
                for (std::int32_t b = top_b; b != GainBuckets::kNone; b = side_d[1].next_desc(b)) {
                    const std::int64_t db = side_d[1].gain(b);
                    if (best_a != GainBuckets::kNone && da + db <= best_gain) break;
                    const std::int64_t gain = da + db - 2 * static_cast<std::int64_t>(w_to_a[b]);
                    if (gain > best_gain) {
                        best_gain = gain;
                        best_a = a;
                        best_b = b;
                    }
                }
                for (std::uint32_t i = g_.begin(a); i < g_.end(a); ++i) w_to_a[g_.target(i)] = 0;
            }

            const auto a = static_cast<NodeId>(best_a);
            const auto b = static_cast<NodeId>(best_b);
            side_d[0].remove(a);
            side_d[1].remove(b);
            move_node(a);
            move_node(b);
            swaps.emplace_back(a, b);

            cumulative += best_gain;
            if (cumulative > best) {
                best = cumulative;
                best_len = swaps.size();
                stall = 0;
            } else if (opts_.stall_limit > 0 && ++stall >= opts_.stall_limit) {
                break;
            }
        }

        // Roll back everything after the best prefix of the swap sequence.
        for (std::size_t i = swaps.size(); i-- > best_len;) {
            p.move(swaps[i].first);
            p.move(swaps[i].second);
        }
        stats.moves += static_cast<std::int64_t>(2 * best_len);
        if (best <= 0) break;
    }
    return stats;
}

}  // namespace cgp

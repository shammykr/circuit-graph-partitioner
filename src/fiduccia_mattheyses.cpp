#include "cgp/fiduccia_mattheyses.hpp"

#include <algorithm>
#include <vector>

#include "cgp/gain_buckets.hpp"

namespace cgp {

RefineStats FiducciaMattheyses::refine(Partition& p) const {
    RefineStats stats;
    const std::size_t n = hg_.num_nodes();
    const std::size_t m = hg_.num_nets();
    if (n < 2) return stats;

    const std::size_t max_side = max_side_size(n, opts_.balance_eps);
    const auto max_gain = static_cast<std::int32_t>(std::max<std::size_t>(hg_.max_node_degree(), 1));
    GainBuckets bucket[2] = {GainBuckets(n, max_gain), GainBuckets(n, max_gain)};
    // Pin count of net e on side s is pins_on[2*e + s]; both counts of a net
    // share a cache line.
    std::vector<std::int32_t> pins_on(2 * m);
    std::vector<NodeId> moves;
    moves.reserve(n);

    auto bump = [&](NodeId u, std::int32_t delta) {
        GainBuckets& b = bucket[p.side(u)];
        if (b.contains(u)) b.adjust(u, delta);
    };

    for (int pass = 0; pass < opts_.max_passes; ++pass) {
        ++stats.passes;
        std::fill(pins_on.begin(), pins_on.end(), 0);
        for (NetId e = 0; e < m; ++e) {
            for (std::uint32_t i = hg_.net_begin(e); i < hg_.net_end(e); ++i) ++pins_on[2 * e + p.side(hg_.pin(i))];
        }
        bucket[0].clear();
        bucket[1].clear();
        for (NodeId v = 0; v < n; ++v) {
            const auto from = p.side(v);
            std::int32_t g = 0;
            for (std::uint32_t i = hg_.node_begin(v); i < hg_.node_end(v); ++i) {
                const NetId e = hg_.incident_net(i);
                if (pins_on[2 * e + from] == 1) ++g;      // moving v uncuts e
                if (pins_on[2 * e + (from ^ 1)] == 0) --g;  // moving v cuts e
            }
            bucket[from].insert(v, g);
        }

        std::int64_t cumulative = 0, best = 0;
        std::size_t best_len = 0, stall = 0;
        moves.clear();

        for (;;) {
            // With unit node weights, if the top node of a side cannot move
            // without breaking balance, no node of that side can.
            std::int32_t cand[2] = {bucket[0].top(), bucket[1].top()};
            if (p.size(1) + 1 > max_side) cand[0] = GainBuckets::kNone;
            if (p.size(0) + 1 > max_side) cand[1] = GainBuckets::kNone;
            if (cand[0] == GainBuckets::kNone && cand[1] == GainBuckets::kNone) break;

            int from;
            if (cand[0] == GainBuckets::kNone) {
                from = 1;
            } else if (cand[1] == GainBuckets::kNone) {
                from = 0;
            } else {
                const auto g0 = bucket[0].gain(cand[0]);
                const auto g1 = bucket[1].gain(cand[1]);
                from = g0 != g1 ? (g0 > g1 ? 0 : 1) : (p.size(0) >= p.size(1) ? 0 : 1);
            }
            const auto v = static_cast<NodeId>(cand[from]);
            const int to = from ^ 1;
            const std::int32_t gain = bucket[from].gain(v);
            bucket[from].remove(v);  // lock v

            for (std::uint32_t i = hg_.node_begin(v); i < hg_.node_end(v); ++i) {
                const NetId e = hg_.incident_net(i);
                std::int32_t* cnt = &pins_on[2 * e];
                const auto nb = hg_.net_begin(e), ne = hg_.net_end(e);
                // Before the move: net becomes cut, or the lone 'to' pin loses its gain.
                if (cnt[to] == 0) {
                    for (auto j = nb; j < ne; ++j) bump(hg_.pin(j), +1);
                } else if (cnt[to] == 1) {
                    for (auto j = nb; j < ne; ++j) {
                        if (p.side(hg_.pin(j)) == to) {
                            bump(hg_.pin(j), -1);
                            break;
                        }
                    }
                }
                --cnt[from];
                ++cnt[to];
                // After the move: net leaves 'from' entirely, or one 'from' pin remains.
                if (cnt[from] == 0) {
                    for (auto j = nb; j < ne; ++j) bump(hg_.pin(j), -1);
                } else if (cnt[from] == 1) {
                    for (auto j = nb; j < ne; ++j) {
                        const NodeId u = hg_.pin(j);
                        if (u != v && p.side(u) == from) {
                            bump(u, +1);
                            break;
                        }
                    }
                }
            }
            p.move(v);
            moves.push_back(v);

            cumulative += gain;
            if (cumulative > best) {
                best = cumulative;
                best_len = moves.size();
                stall = 0;
            } else if (opts_.stall_limit > 0 && ++stall >= opts_.stall_limit) {
                break;
            }
        }

        for (std::size_t i = moves.size(); i-- > best_len;) p.move(moves[i]);
        stats.moves += static_cast<std::int64_t>(best_len);
        if (best <= 0) break;
    }
    return stats;
}

}  // namespace cgp

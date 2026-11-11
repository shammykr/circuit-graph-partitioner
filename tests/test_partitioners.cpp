#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <vector>

#include "cgp/fiduccia_mattheyses.hpp"
#include "cgp/graph.hpp"
#include "cgp/hypergraph.hpp"
#include "cgp/kernighan_lin.hpp"
#include "cgp/multistart.hpp"

using namespace cgp;

namespace {

// Two k-node cliques joined by a single bridge edge (k-1 -> k).
Graph two_cliques(NodeId k) {
    std::vector<Edge> edges;
    for (NodeId base : {NodeId{0}, k}) {
        for (NodeId i = 0; i < k; ++i) {
            for (NodeId j = i + 1; j < k; ++j) edges.push_back({base + i, base + j, 1});
        }
    }
    edges.push_back({k - 1, k, 1});
    return Graph(2 * k, edges);
}

// Two clusters of k nodes, each held together by a chain of 3-pin nets,
// plus one net spanning both clusters.
Hypergraph two_clusters(NodeId k) {
    std::vector<std::vector<NodeId>> nets;
    for (NodeId base : {NodeId{0}, k}) {
        for (NodeId i = 0; i + 2 < k; ++i) nets.push_back({base + i, base + i + 1, base + i + 2});
        nets.push_back({base, base + k - 1});
    }
    nets.push_back({k - 1, k});
    return Hypergraph(2 * k, nets);
}

Hypergraph random_hypergraph(std::size_t n, std::size_t m, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::vector<std::vector<NodeId>> nets(m);
    for (auto& net : nets) {
        const std::size_t k = 2 + rng() % 4;
        for (std::size_t j = 0; j < k; ++j) net.push_back(static_cast<NodeId>(rng() % n));
    }
    return Hypergraph(n, nets);
}

// The two halves of the planted bisection must end up on different sides.
void expect_halves_separated(const Partition& p, NodeId k) {
    for (NodeId v = 1; v < k; ++v) EXPECT_EQ(p.side(v), p.side(0));
    for (NodeId v = k; v < 2 * k; ++v) EXPECT_NE(p.side(v), p.side(0));
}

}  // namespace

TEST(KernighanLin, FindsPlantedBisectionOfTwoCliques) {
    const Graph g = two_cliques(8);
    KernighanLin kl(g);
    std::mt19937_64 rng(42);
    Partition p = Partition::random_balanced(g.num_nodes(), rng);
    kl.refine(p);
    EXPECT_EQ(g.cut(p), 1);
    EXPECT_EQ(p.size(0), 8u);
    EXPECT_EQ(p.size(1), 8u);
    expect_halves_separated(p, 8);
}

TEST(KernighanLin, KeepsExactBalanceAndNeverWorsensCut) {
    const Graph g = clique_expand(random_hypergraph(301, 600, 7), 16);
    KernighanLin kl(g);
    for (std::uint64_t seed = 0; seed < 5; ++seed) {
        std::mt19937_64 rng(seed);
        Partition p = Partition::random_balanced(g.num_nodes(), rng);
        const auto before = g.cut(p);
        kl.refine(p);
        EXPECT_LE(g.cut(p), before);
        EXPECT_EQ(p.size(0), 150u);  // odd n: floor(n/2) on side 0 is preserved
        EXPECT_EQ(p.size(1), 151u);
    }
}

TEST(FiducciaMattheyses, FindsPlantedBisectionOfTwoClusters) {
    const Hypergraph hg = two_clusters(10);
    FiducciaMattheyses fm(hg);
    std::mt19937_64 rng(3);
    Partition p = Partition::random_balanced(hg.num_nodes(), rng);
    fm.refine(p);
    EXPECT_EQ(hg.cut(p), 1);
    EXPECT_TRUE(p.is_balanced(0.02));
    expect_halves_separated(p, 10);
}

TEST(FiducciaMattheyses, RespectsBalanceToleranceAndNeverWorsensCut) {
    const Hypergraph hg = random_hypergraph(1000, 2000, 11);
    for (double eps : {0.0, 0.05, 0.2}) {
        FMOptions opts;
        opts.balance_eps = eps;
        FiducciaMattheyses fm(hg, opts);
        std::mt19937_64 rng(5);
        Partition p = Partition::random_balanced(hg.num_nodes(), rng);
        const auto before = hg.cut(p);
        fm.refine(p);
        EXPECT_LT(hg.cut(p), before);
        EXPECT_TRUE(p.is_balanced(eps));
        EXPECT_LE(std::max(p.size(0), p.size(1)), max_side_size(hg.num_nodes(), eps));
        EXPECT_EQ(p.size(0) + p.size(1), hg.num_nodes());
    }
}

TEST(MultiStart, KeepsBestStartAndIsThreadCountIndependent) {
    const Hypergraph hg = random_hypergraph(500, 1000, 13);
    FiducciaMattheyses fm(hg);
    MultiStartOptions one{12, 1, 99};
    MultiStartOptions many{12, 4, 99};
    const MultiStartResult a = run_multistart(fm, one);
    const MultiStartResult b = run_multistart(fm, many);

    ASSERT_EQ(a.costs.size(), 12u);
    EXPECT_EQ(a.best_cost, *std::min_element(a.costs.begin(), a.costs.end()));
    EXPECT_EQ(a.best_cost, hg.cut(a.best));
    EXPECT_TRUE(a.best.is_balanced(0.02));
    EXPECT_EQ(a.costs, b.costs);
    EXPECT_EQ(a.best.sides(), b.best.sides());
}

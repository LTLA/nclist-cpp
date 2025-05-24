#include <gtest/gtest.h>

#include <vector>
#include <random>
#include <cstddef>

#include "nclist/overlaps_any.hpp"
#include "nclist/overlaps_equal.hpp"
#include "utils.hpp"

TEST(OverlapsEqual, Empty) {
    auto index = nclist::build<int, int>(0, NULL, NULL);
    nclist::OverlapsEqualParameters<int> params;
    nclist::OverlapsEqualWorkspace<int> workspace;
    std::vector<int> output;
    nclist::overlaps_equal(index, 100, 200, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

/********************************************************************/

TEST(OverlapsEqual, SimpleDisjoint) {
    std::vector<int> test_starts { 10, 30, 50, 0 };
    std::vector<int> test_ends   { 20, 45, 70, 5 };
    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsEqualParameters<int> params;
    nclist::OverlapsEqualWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_equal(index, 10, 30, params, workspace, output);
    EXPECT_TRUE(output.empty());

    nclist::overlaps_equal(index, 10, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);

    nclist::overlaps_equal(index, 50, 70, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    nclist::overlaps_equal(index, 12, 13, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsEqual, SimpleOverlaps) {
    std::vector<int> test_starts { 10, 30, 40,  0,  5 };
    std::vector<int> test_ends   { 50, 65, 70, 20, 30 };
    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsEqualParameters<int> params;
    nclist::OverlapsEqualWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_equal(index, 10, 30, params, workspace, output);
    EXPECT_TRUE(output.empty());

    nclist::overlaps_equal(index, 0, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 3);

    nclist::overlaps_equal(index, 5, 30, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 4);
}

TEST(OverlapsEqual, SimpleNested) {
    std::vector<int> test_starts { 10, 30, 20,   0, 50, 50, 70 };
    std::vector<int> test_ends   { 50, 45, 50, 100, 60, 80, 80 };
    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsEqualParameters<int> params;
    nclist::OverlapsEqualWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_equal(index, 50, 60, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 4);

    nclist::overlaps_equal(index, 0, 100, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 3);

    nclist::overlaps_equal(index, 30, 45, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);

    nclist::overlaps_equal(index, 50, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 5);

    nclist::overlaps_equal(index, 70, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 6);
}

TEST(OverlapsEqual, SimpleMaxGap) {
    std::vector<int> test_starts { 10, 30, 20,   0, 50, 50, 70 };
    std::vector<int> test_ends   { 50, 45, 50, 100, 60, 80, 80 };
    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsEqualParameters<int> params;
    nclist::OverlapsEqualWorkspace<int> workspace;
    std::vector<int> output;

    params.max_gap = 5;
    nclist::overlaps_equal(index, 25, 45, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 1);
    EXPECT_EQ(output[1], 2);

    nclist::overlaps_equal(index, 15, 45, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 2);

    nclist::overlaps_equal(index, 40, 42, params, workspace, output);
    EXPECT_TRUE(output.empty());

    params.max_gap = 10;
    nclist::overlaps_equal(index, 60, 70, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 4);
    EXPECT_EQ(output[1], 5);
    EXPECT_EQ(output[2], 6);

    params.max_gap = 15;
    nclist::overlaps_equal(index, 35, 65, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 2);
    EXPECT_EQ(output[1], 4);
    EXPECT_EQ(output[2], 5);
}

TEST(OverlapsEqual, SimpleMinOverlap) {
    std::vector<int> test_starts { 10, 30, 20,   0, 50, 50, 70 };
    std::vector<int> test_ends   { 50, 45, 50, 100, 60, 80, 80 };
    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsEqualParameters<int> params;
    nclist::OverlapsEqualWorkspace<int> workspace;
    std::vector<int> output;

    params.min_overlap = 20;
    nclist::overlaps_equal(index, 30, 45, params, workspace, output);
    EXPECT_TRUE(output.empty());

    nclist::overlaps_equal(index, 0, 100, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 3);

    nclist::overlaps_equal(index, 50, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 5);
}

TEST(OverlapsEqual, SimpleMinOverlapAndMaxGap) {
    std::vector<int> test_starts { 10, 30, 20,   0, 50, 50, 70 };
    std::vector<int> test_ends   { 50, 45, 50, 100, 60, 80, 80 };
    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsEqualParameters<int> params;
    nclist::OverlapsEqualWorkspace<int> workspace;
    std::vector<int> output;

    params.max_gap = 5;
    nclist::overlaps_equal(index, 25, 45, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    params.min_overlap = 20;
    nclist::overlaps_equal(index, 25, 45, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    params.max_gap = 20;
    params.min_overlap = 10;
    nclist::overlaps_equal(index, 40, 70, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 2);
    EXPECT_EQ(output[1], 4);
    EXPECT_EQ(output[2], 5);
}

/********************************************************************/

class OverlapsEqualReferenceTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int> > {
protected:
    void SetUp() {
        assemble(GetParam());
    }
};

TEST_P(OverlapsEqualReferenceTest, Basic) {
    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());

    nclist::OverlapsAnyWorkspace<int> a_work;
    nclist::OverlapsAnyParameters<int> a_params;
    std::vector<int> filtered;

    nclist::OverlapsEqualWorkspace<int> w_work;
    nclist::OverlapsEqualParameters<int> w_params;
    std::vector<int> results;

    for (int q = 0; q < nquery; ++q) {
        auto qstart = query_start[q];
        auto qend = query_end[q];

        nclist::overlaps_any(index, qstart, qend, a_params, a_work, results);
        filtered.clear();
        for (auto r : results) {
            if (qstart == subject_start[r] && qend == subject_end[r]) {
                filtered.push_back(r);
            }
        }
        std::sort(filtered.begin(), filtered.end());

        nclist::overlaps_equal(index, query_start[q], query_end[q], w_params, w_work, results);
        std::sort(results.begin(), results.end());
        EXPECT_EQ(results, filtered);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapsEqual,
    OverlapsEqualReferenceTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000) // number of subject ranges
    )
);

class OverlapsEqualComplicatedTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int, int, int> > {
protected:
    int min_overlap, max_gap;
    void SetUp() {
        auto params = GetParam();
        assemble(params);
        min_overlap = std::get<2>(params);
        max_gap = std::get<3>(params);
    }
};

TEST_P(OverlapsEqualComplicatedTest, Basic) {
    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());

    nclist::OverlapsAnyWorkspace<int> a_work;
    nclist::OverlapsAnyParameters<int> a_params;
    a_params.min_overlap = min_overlap;
    a_params.max_gap = max_gap;
    std::vector<int> filtered;

    nclist::OverlapsEqualWorkspace<int> w_work;
    nclist::OverlapsEqualParameters<int> w_params;
    w_params.min_overlap = min_overlap;
    w_params.max_gap = max_gap;
    std::vector<int> results;

    for (int q = 0; q < nquery; ++q) {
        auto qstart = query_start[q];
        auto qend = query_end[q];

        nclist::overlaps_any(index, qstart, qend, a_params, a_work, results);
        filtered.clear();
        for (auto r : results) {
            if (min_overlap > 0) {
                if (std::min(qend, subject_end[r]) - std::max(qstart, subject_start[r]) < min_overlap) {
                    continue;
                }
            }
            if (std::abs(qend - subject_end[r]) > max_gap || std::abs(qstart - subject_start[r]) > max_gap) {
                continue;
            } 
            filtered.push_back(r);
        }
        std::sort(filtered.begin(), filtered.end());

        nclist::overlaps_equal(index, query_start[q], query_end[q], w_params, w_work, results);
        std::sort(results.begin(), results.end());
        EXPECT_EQ(results, filtered);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapsEqual,
    OverlapsEqualComplicatedTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000), // number of subject ranges
        ::testing::Values(5, 10, 20), // min overlap
        ::testing::Values(5, 10, 20) // max gap 
    )
);

/********************************************************************/

TEST(OverlapsEqual, Unsigned) {
    std::vector<unsigned> test_starts { 200, 300, 100, 500 };
    std::vector<unsigned> test_ends { 280, 320, 170, 510 };
    auto index = nclist::build<std::size_t, unsigned>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsEqualParameters<unsigned> params;
    nclist::OverlapsEqualWorkspace<std::size_t> workspace;
    std::vector<std::size_t> output;

    nclist::overlaps_equal(index, 200u, 280u, nclist::OverlapsEqualParameters<unsigned>(), workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);

    nclist::overlaps_equal(index, 100u, 170u, nclist::OverlapsEqualParameters<unsigned>(), workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    // Behaves with a max gap.
    params.max_gap = 60; // check that we avoid underflow when defining the search start.
    nclist::overlaps_equal(index, 50u, 200u, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    // Behaves with a minimum overlap.
    params.min_overlap = 200; 
    nclist::overlaps_equal(index, 100u, 170u, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsEqual, Double) {
    std::vector<double> test_starts { 200.5, 300.1, 100.8, 500.5 };
    std::vector<double> test_ends { 280.2, 320.9, 170.1, 510.5 };
    auto index = nclist::build<int, double>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsEqualParameters<double> params;
    nclist::OverlapsEqualWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_equal(index, 100.8, 170.1, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    nclist::overlaps_equal(index, 101.0, 170.0, params, workspace, output);
    EXPECT_TRUE(output.empty());
    params.max_gap = 0.5;
    nclist::overlaps_equal(index, 101.0, 170.0, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    params.max_gap = 0;
    nclist::overlaps_equal(index, 500.5, 510.5, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 3);
    params.min_overlap = 10.5;
    nclist::overlaps_equal(index, 500.5, 510.5, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

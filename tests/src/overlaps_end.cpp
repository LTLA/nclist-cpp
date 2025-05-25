#include <gtest/gtest.h>

#include <vector>
#include <random>
#include <algorithm>

#include "nclist/overlaps_end.hpp"
#include "nclist/overlaps_any.hpp"
#include "utils.hpp"

TEST(OverlapsEnd, Empty) {
    auto index = nclist::build<int, int>(0, NULL, NULL);
    nclist::OverlapsEndWorkspace<int> workspace;
    std::vector<int> output;
    nclist::overlaps_end(index, 100, 200, nclist::OverlapsEndParameters<int>(), workspace, output);
    EXPECT_TRUE(output.empty());
}

/********************************************************************/

TEST(OverlapsEnd, SimpleDisjoint) {
    std::vector<int> starts{ 16, 84, 32, 77,  6 };
    std::vector<int> ends  { 25, 96, 45, 80, 13 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsEndParameters<int> params;
    nclist::OverlapsEndWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_end(index, 10, 25, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);

    nclist::overlaps_end(index, 16, 30, params, workspace, output);
    EXPECT_TRUE(output.empty());

    nclist::overlaps_end(index, 84, 96, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);

    nclist::overlaps_end(index, 33, 44, params, workspace, output);
    EXPECT_TRUE(output.empty());

    nclist::overlaps_end(index, 5, 20, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsEnd, SimpleOverlaps) {
    std::vector<int> starts{ 16, 32, 24,  7,  0 };
    std::vector<int> ends  { 50, 75, 66, 40, 20 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsEndParameters<int> params;
    nclist::OverlapsEndWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_end(index, 16, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 4);

    nclist::overlaps_end(index, 35, 75, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);

    nclist::overlaps_end(index, -10, 66, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    nclist::overlaps_end(index, 33, 44, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsEnd, SimpleNested) {
    std::vector<int> starts{ 16, 25, 20,  0, 30, 30, 10 };
    std::vector<int> ends  { 20, 50, 50, 80, 80, 60, 30 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsEndParameters<int> params;
    nclist::OverlapsEndWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_end(index, 5, 30, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 6);

    nclist::overlaps_end(index, 30, 50, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 1);
    EXPECT_EQ(output[1], 2);

    nclist::overlaps_end(index, 70, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 3);
    EXPECT_EQ(output[1], 4);
}

TEST(OverlapsEnd, SimpleMaxGap) {
    std::vector<int> starts{ 16, 25, 20,  0, 30, 30, 10 };
    std::vector<int> ends  { 20, 50, 50, 80, 80, 60, 30 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsEndParameters<int> params;
    nclist::OverlapsEndWorkspace<int> workspace;
    std::vector<int> output;

    params.max_gap = 5;
    nclist::overlaps_end(index, 20, 55, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 1);
    EXPECT_EQ(output[1], 2);
    EXPECT_EQ(output[2], 5);

    params.max_gap = 20;
    nclist::overlaps_end(index, 0, 40, params, workspace, output);
    ASSERT_EQ(output.size(), 5);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);
    EXPECT_EQ(output[2], 2);
    EXPECT_EQ(output[3], 5);
    EXPECT_EQ(output[4], 6);

    params.max_gap = 20;
    nclist::overlaps_end(index, 6, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 3);
    EXPECT_EQ(output[1], 4);
    EXPECT_EQ(output[2], 5);
}

TEST(OverlapsEnd, SimpleMinOverlap) {
    std::vector<int> starts{ 16, 25, 20,  0, 30, 30, 10 };
    std::vector<int> ends  { 20, 50, 50, 80, 80, 60, 30 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsEndParameters<int> params;
    nclist::OverlapsEndWorkspace<int> workspace;
    std::vector<int> output;

    params.min_overlap = 0;
    nclist::overlaps_end(index, 22, 50, params, workspace, output);
    EXPECT_EQ(output.size(), 2);
    params.min_overlap = 26;
    nclist::overlaps_end(index, 22, 50, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    params.min_overlap = 0;
    nclist::overlaps_end(index, 20, 80, params, workspace, output);
    EXPECT_EQ(output.size(), 2);
    params.min_overlap = 60;
    nclist::overlaps_end(index, 20, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 3);

    // Check the early return.
    params.min_overlap = 100;
    nclist::overlaps_end(index, 0, 80, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsEnd, MaxGapAndMinOverlap) {
    std::vector<int> starts{ 16, 25, 20,  0, 30, 30, 10 };
    std::vector<int> ends  { 20, 50, 50, 80, 80, 60, 30 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsEndParameters<int> params;
    nclist::OverlapsEndWorkspace<int> workspace;
    std::vector<int> output;

    params.max_gap = 10;
    nclist::overlaps_end(index, 45, 55, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    params.min_overlap = 10;
    nclist::overlaps_end(index, 45, 55, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 5);

    params.max_gap = 10;
    params.min_overlap = 0;
    nclist::overlaps_end(index, 45, 55, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    params.min_overlap = 35;
    nclist::overlaps_end(index, 30, 70, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 3);
    EXPECT_EQ(output[1], 4);

    params.max_gap = 5;
    params.min_overlap = 0;
    nclist::overlaps_end(index, 15, 25, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    params.min_overlap = 10;
    nclist::overlaps_end(index, 15, 25, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 6);
}

/********************************************************************/

class OverlapsEndRandomizedTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int, int, int> > {
protected:
    int min_overlap, max_gap;
    void SetUp() {
        auto params = GetParam();
        assemble(params);
        min_overlap = std::get<2>(params);
        max_gap = std::get<3>(params);
    }
};

TEST_P(OverlapsEndRandomizedTest, Basic) {
    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());

    nclist::OverlapsAnyWorkspace<int> a_work;
    nclist::OverlapsAnyParameters<int> a_params;
    std::vector<int> filtered;

    nclist::OverlapsEndWorkspace<int> w_work;
    nclist::OverlapsEndParameters<int> w_params;
    w_params.min_overlap = min_overlap;
    w_params.max_gap = max_gap;
    std::vector<int> results;

    for (int q = 0; q < nquery; ++q) {
        auto qstart = query_start[q];
        auto qend = query_end[q];

        nclist::overlaps_any(index, qstart - max_gap, qend + max_gap, a_params, a_work, results);
        filtered.clear();
        for (auto r : results) {
            if (min_overlap > 0) {
                if (std::min(qend, subject_end[r]) - std::max(qstart, subject_start[r]) < min_overlap) {
                    continue;
                }
            }
            if (max_gap > 0) {
                if (std::abs(qend - subject_end[r]) > max_gap) {
                    continue;
                } 
            } else {
                if (qend != subject_end[r]) {
                    continue;
                }
            }
            filtered.push_back(r);
        }
        std::sort(filtered.begin(), filtered.end());

        nclist::overlaps_end(index, query_start[q], query_end[q], w_params, w_work, results);
        std::sort(results.begin(), results.end());
        EXPECT_EQ(results, filtered);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapsEnd,
    OverlapsEndRandomizedTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000), // number of subject ranges
        ::testing::Values(0, 5, 10, 20), // min overlap
        ::testing::Values(0, 5, 10, 20) // max gap 
    )
);

/********************************************************************/

TEST(OverlapsEnd, Unsigned) {
    std::vector<unsigned> test_starts{ 200, 300, 100, 500 };
    std::vector<unsigned> test_ends  { 280, 320, 170, 510 };
    auto index = nclist::build<std::size_t, unsigned>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsEndParameters<unsigned> params;
    nclist::OverlapsEndWorkspace<std::size_t> workspace;
    std::vector<std::size_t> output;

    nclist::overlaps_end(index, 150u, 280u, nclist::OverlapsEndParameters<unsigned>(), workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);

    nclist::overlaps_end(index, 20u, 170u, nclist::OverlapsEndParameters<unsigned>(), workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    // Behaves with a max gap.
    params.max_gap = 150; // check that we avoid underflow when defining the new search end.
    nclist::overlaps_end(index, 50u, 100u, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);
}

TEST(OverlapsEnd, Double) {
    std::vector<double> test_starts{ 200.5, 300.1, 100.8, 500.5 };
    std::vector<double> test_ends  { 280.2, 320.9, 170.1, 510.5 };
    auto index = nclist::build<int, double>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsEndParameters<double> params;
    nclist::OverlapsEndWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_end(index, 150.8, 170.1, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    nclist::overlaps_end(index, 121.0, 170.0, params, workspace, output);
    EXPECT_TRUE(output.empty());
    params.max_gap = 0.5;
    nclist::overlaps_end(index, 121.0, 170.0, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    params.max_gap = 0;
    nclist::overlaps_end(index, 490.5, 510.5, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 3);
    params.min_overlap = 10.5;
    nclist::overlaps_end(index, 490.5, 510.5, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

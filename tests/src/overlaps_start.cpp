#include <gtest/gtest.h>

#include <vector>
#include <random>
#include <algorithm>

#include "nclist/overlaps_start.hpp"
#include "nclist/overlaps_any.hpp"
#include "utils.hpp"

TEST(OverlapsStart, Empty) {
    auto index = nclist::build<int, int>(0, NULL, NULL);
    nclist::OverlapsStartWorkspace<int> workspace;
    std::vector<int> output;
    nclist::overlaps_start(index, 100, 200, nclist::OverlapsStartParameters<int>(), workspace, output);
    EXPECT_TRUE(output.empty());
}

/********************************************************************/

TEST(OverlapsStart, SimpleDisjoint) {
    std::vector<int> starts{ 16, 84, 32, 77,  6 };
    std::vector<int> ends  { 25, 96, 45, 80, 13 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsStartParameters<int> params;
    nclist::OverlapsStartWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_start(index, 16, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);

    nclist::overlaps_start(index, 0, 25, params, workspace, output);
    EXPECT_TRUE(output.empty());

    nclist::overlaps_start(index, 84, 96, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);

    nclist::overlaps_start(index, 33, 44, params, workspace, output);
    EXPECT_TRUE(output.empty());

    nclist::overlaps_start(index, 5, 20, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsStart, SimpleOverlaps) {
    std::vector<int> starts{ 16, 32, 24,  7,  0 };
    std::vector<int> ends  { 50, 75, 66, 40, 20 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsStartParameters<int> params;
    nclist::OverlapsStartWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_start(index, 16, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);

    nclist::overlaps_start(index, 32, 45, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);

    nclist::overlaps_start(index, 0, 96, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 4);

    nclist::overlaps_start(index, 33, 44, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsStart, SimpleNested) {
    std::vector<int> starts{ 16, 25, 20,  0, 30, 30,  16 };
    std::vector<int> ends  { 20, 35, 50, 80, 66, 60,  30 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsStartParameters<int> params;
    nclist::OverlapsStartWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_start(index, 16, 18, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 6);

    nclist::overlaps_start(index, 30, 40, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 4);
    EXPECT_EQ(output[1], 5);

    nclist::overlaps_start(index, 25, 30, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);
}

TEST(OverlapsStart, SimpleMaxGap) {
    std::vector<int> starts{ 16, 25, 20,  0, 30, 30, 16 };
    std::vector<int> ends  { 20, 35, 50, 80, 66, 60, 30 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsStartParameters<int> params;
    nclist::OverlapsStartWorkspace<int> workspace;
    std::vector<int> output;

    params.max_gap = 2;
    nclist::overlaps_start(index, 18, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 2);
    EXPECT_EQ(output[2], 6);

    params.max_gap = 5;
    nclist::overlaps_start(index, 25, 30, params, workspace, output);
    ASSERT_EQ(output.size(), 4);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 1);
    EXPECT_EQ(output[1], 2);
    EXPECT_EQ(output[2], 4);
    EXPECT_EQ(output[3], 5);

    params.max_gap = 10;
    nclist::overlaps_start(index, 6, 30, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 3);
    EXPECT_EQ(output[2], 6);
}

TEST(OverlapsStart, SimpleMinOverlap) {
    std::vector<int> starts{ 16, 25, 20,  0, 30, 30, 16 };
    std::vector<int> ends  { 20, 35, 50, 80, 66, 60, 30 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsStartParameters<int> params;
    nclist::OverlapsStartWorkspace<int> workspace;
    std::vector<int> output;

    params.min_overlap = 0;
    nclist::overlaps_start(index, 16, 26, params, workspace, output);
    EXPECT_EQ(output.size(), 2);
    params.min_overlap = 10;
    nclist::overlaps_start(index, 16, 26, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 6);

    params.min_overlap = 0;
    nclist::overlaps_start(index, 30, 100, params, workspace, output);
    EXPECT_EQ(output.size(), 2);
    params.min_overlap = 35;
    nclist::overlaps_start(index, 30, 100, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 4);

    params.min_overlap = 0;
    nclist::overlaps_start(index, 0, 100, params, workspace, output);
    EXPECT_EQ(output.size(), 1);
    params.min_overlap = 100;
    nclist::overlaps_start(index, 0, 100, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsStart, MaxGapAndMinOverlap) {
    std::vector<int> starts{ 16, 25, 20,  0, 30, 30, 16 };
    std::vector<int> ends  { 20, 35, 50, 80, 66, 60, 30 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsStartParameters<int> params;
    nclist::OverlapsStartWorkspace<int> workspace;
    std::vector<int> output;

    params.max_gap = 10;
    params.min_overlap = 10;
    nclist::overlaps_start(index, 18, 30, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 2);
    EXPECT_EQ(output[1], 6);

    params.max_gap = 5;
    params.min_overlap = 30;
    nclist::overlaps_start(index, 25, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 4);
    EXPECT_EQ(output[1], 5);
    params.min_overlap = 35;
    nclist::overlaps_start(index, 25, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 4);
}

/********************************************************************/

class OverlapsStartRandomizedTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int, int, int> > {
protected:
    int min_overlap, max_gap;
    void SetUp() {
        auto params = GetParam();
        assemble(params);
        min_overlap = std::get<2>(params);
        max_gap = std::get<3>(params);
    }
};

TEST_P(OverlapsStartRandomizedTest, Basic) {
    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());

    nclist::OverlapsAnyWorkspace<int> a_work;
    nclist::OverlapsAnyParameters<int> a_params;
    std::vector<int> filtered;

    nclist::OverlapsStartWorkspace<int> w_work;
    nclist::OverlapsStartParameters<int> w_params;
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
                if (std::abs(qstart - subject_start[r]) > max_gap) {
                    continue;
                } 
            } else {
                if (qstart != subject_start[r]) {
                    continue;
                }
            }
            filtered.push_back(r);
        }
        std::sort(filtered.begin(), filtered.end());

        nclist::overlaps_start(index, query_start[q], query_end[q], w_params, w_work, results);
        std::sort(results.begin(), results.end());
        EXPECT_EQ(results, filtered);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapsStart,
    OverlapsStartRandomizedTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000), // number of subject ranges
        ::testing::Values(0, 5, 10, 20), // min overlap
        ::testing::Values(0, 5, 10, 20) // max gap 
    )
);

/********************************************************************/

TEST(OverlapsStart, Unsigned) {
    std::vector<unsigned> test_starts { 200, 300, 100, 500 };
    std::vector<unsigned> test_ends { 280, 320, 170, 510 };
    auto index = nclist::build<std::size_t, unsigned>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsStartParameters<unsigned> params;
    nclist::OverlapsStartWorkspace<std::size_t> workspace;
    std::vector<std::size_t> output;

    nclist::overlaps_start(index, 200u, 300u, nclist::OverlapsStartParameters<unsigned>(), workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);

    nclist::overlaps_start(index, 100u, 400u, nclist::OverlapsStartParameters<unsigned>(), workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    // Behaves with a max gap.
    params.max_gap = 60; // check that we avoid underflow when defining the search start.
    nclist::overlaps_start(index, 50u, 200u, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    // Check that we return early if a huge overlap is specified.
    params.min_overlap = std::numeric_limits<unsigned>::max();
    nclist::overlaps_start(index, 100u, 170u, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsStart, Double) {
    std::vector<double> test_starts { 200.5, 300.1, 100.8, 500.5 };
    std::vector<double> test_ends { 280.2, 320.9, 170.1, 510.5 };
    auto index = nclist::build<int, double>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsStartParameters<double> params;
    nclist::OverlapsStartWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_start(index, 100.8, 990.1, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    nclist::overlaps_start(index, 101.0, 180.0, params, workspace, output);
    EXPECT_TRUE(output.empty());
    params.max_gap = 0.5;
    nclist::overlaps_start(index, 101.0, 190.0, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    params.max_gap = 0;
    nclist::overlaps_start(index, 500.5, 520.5, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 3);
    params.min_overlap = 10.5;
    nclist::overlaps_start(index, 500.5, 520.5, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

/********************************************************************/

TEST(OverlapsStart, Duplicates) {
    std::vector<int> starts{ 16, 16, 84, 32, 77, 77,  6 };
    std::vector<int> ends  { 25, 25, 96, 45, 80, 80, 13 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsStartParameters<int> params;
    nclist::OverlapsStartWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_start(index, 16, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);

    params.max_gap = 10;
    nclist::overlaps_start(index, 80, 100, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 2);
    EXPECT_EQ(output[1], 4);
    EXPECT_EQ(output[2], 5);

    nclist::overlaps_start(index, 10, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);
    EXPECT_EQ(output[2], 6);
}

TEST(OverlapsStart, EarlyQuit) {
    std::vector<int> starts{ 16, 84, 32, 77,  6 };
    std::vector<int> ends  { 25, 96, 45, 80, 13 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsStartParameters<int> params;
    nclist::OverlapsStartWorkspace<int> workspace;
    params.quit_on_first = true;
    std::vector<int> output;

    params.max_gap = 10;
    nclist::overlaps_start(index, 16, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_TRUE(output[0] == 0 || output[0] == 4);

    nclist::overlaps_start(index, 80, 100, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_TRUE(output[0] == 1 || output[0] == 3);

    params.max_gap = 0;
    nclist::overlaps_start(index, 0, 100, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsStart, ZeroWidth) {
    std::vector<int> test_starts { 200, 400 };
    std::vector<int> test_ends { 200, 500 };
    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsStartWorkspace<int> workspace;
    nclist::OverlapsStartParameters<int> params;
    std::vector<int> output;

    nclist::overlaps_start(index, 200, 200, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);
    nclist::overlaps_start(index, 400, 400, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);

    nclist::overlaps_start(index, 200, 200, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);

    params.max_gap = 10;
    nclist::overlaps_start(index, 190, 300, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);
    nclist::overlaps_start(index, 410, 410, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);

    params.min_overlap = 1;
    nclist::overlaps_start(index, 200, 200, params, workspace, output);
    EXPECT_TRUE(output.empty());
    nclist::overlaps_start(index, 400, 400, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

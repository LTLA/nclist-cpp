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

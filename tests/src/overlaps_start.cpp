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


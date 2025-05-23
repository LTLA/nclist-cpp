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


#include <gtest/gtest.h>

#include <vector>

#include "nclist/overlaps_any.hpp"
#include "utils.hpp"

TEST(OverlapsAny, Empty) {
    auto index = nclist::build<int, int>(0, NULL, NULL);
    nclist::OverlapsAnyWorkspace<int> workspace;
    std::vector<int> output;
    nclist::overlaps_any(index, 100, 200, nclist::OverlapsAnyParameters<int>(), workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsAny, SimpleDisjoint) {
    std::vector<int> test_starts { 200, 300, 100, 500 };
    std::vector<int> test_ends { 280, 320, 170, 510 };

    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::OverlapsAnyWorkspace<int> workspace;
    std::vector<int> output;

    {
        nclist::overlaps_any(index, 150, 200, nclist::OverlapsAnyParameters<int>(), workspace, output);
        EXPECT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::overlaps_any(index, 150, 300, nclist::OverlapsAnyParameters<int>(), workspace, output);
        EXPECT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 2);
    }

    {
        nclist::overlaps_any(index, 210, 310, nclist::OverlapsAnyParameters<int>(), workspace, output);
        EXPECT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
    }

    {
        nclist::overlaps_any(index, 90, 600, nclist::OverlapsAnyParameters<int>(), workspace, output);
        EXPECT_EQ(output.size(), 4);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 2);
        EXPECT_EQ(output[3], 3);
    }
}

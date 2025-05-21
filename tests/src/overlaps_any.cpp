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
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::overlaps_any(index, 150, 300, nclist::OverlapsAnyParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 2);
    }

    {
        nclist::overlaps_any(index, 210, 310, nclist::OverlapsAnyParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
    }

    {
        nclist::overlaps_any(index, 90, 600, nclist::OverlapsAnyParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 4);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 2);
        EXPECT_EQ(output[3], 3);
    }
}

TEST(OverlapsAny, SimpleOverlaps) {
    std::vector<int> test_starts { 200, 300, 100, 500 };
    std::vector<int> test_ends { 600, 720, 510, 1000 };

    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::OverlapsAnyWorkspace<int> workspace;
    std::vector<int> output;

    {
        nclist::overlaps_any(index, 150, 200, nclist::OverlapsAnyParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::overlaps_any(index, 50, 400, nclist::OverlapsAnyParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 3);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 2);
    }

    {
        nclist::overlaps_any(index, 700, 1000, nclist::OverlapsAnyParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 1);
        EXPECT_EQ(output[1], 3);
    }

    {
        nclist::overlaps_any(index, 500, 600, nclist::OverlapsAnyParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 4);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 2);
        EXPECT_EQ(output[3], 3);
    }
}

TEST(OverlapsAny, SimpleNested) {
    std::vector<int> test_starts { 0, 20, 20, 40, 70, 90 };
    std::vector<int> test_ends { 100, 60, 30, 50, 95, 95 };

    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::OverlapsAnyWorkspace<int> workspace;
    std::vector<int> output;

    {
        nclist::overlaps_any(index, 0, 10, nclist::OverlapsAnyParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 0);
    }

    {
        nclist::overlaps_any(index, 42, 45, nclist::OverlapsAnyParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 3);
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 3);
    }

    {
        nclist::overlaps_any(index, 35, 40, nclist::OverlapsAnyParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
    }

    {
        nclist::overlaps_any(index, 45, 80, nclist::OverlapsAnyParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 4);
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 3);
        EXPECT_EQ(output[3], 4);
    }
}

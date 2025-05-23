#include <gtest/gtest.h>

#include <vector>
#include <random>
#include <algorithm>

#include "nclist/overlaps_within.hpp"
#include "nclist/overlaps_any.hpp"
#include "utils.hpp"

TEST(OverlapsWithin, Empty) {
    auto index = nclist::build<int, int>(0, NULL, NULL);
    nclist::OverlapsWithinWorkspace<int> workspace;
    std::vector<int> output;
    nclist::overlaps_within(index, 100, 200, nclist::OverlapsWithinParameters<int>(), workspace, output);
    EXPECT_TRUE(output.empty());
}

/********************************************************************/

TEST(OverlapsWithin, SimpleDisjoint) {
    std::vector<int> starts{ 10, 102, 35, 71, 0 };
    std::vector<int> ends  { 20, 145, 55, 78, 8 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsWithinParameters<int> params;
    nclist::OverlapsWithinWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_within(index, 105, 140, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);

    nclist::overlaps_within(index, 0, 8, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 4);

    nclist::overlaps_within(index, 35, 40, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);

    nclist::overlaps_within(index, 75, 78, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 3);

    nclist::overlaps_within(index, 0, 20, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsWithin, SimpleMinOverlap) {
    std::vector<int> starts{ 10 };
    std::vector<int> ends{ 20 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsWithinParameters<int> params;
    nclist::OverlapsWithinWorkspace<int> workspace;
    std::vector<int> output;

    // Testing with default parameters as a control.
    nclist::overlaps_within(index, 10, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);
    nclist::overlaps_within(index, 15, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 1);

    params.min_overlap = 10;
    nclist::overlaps_within(index, 10, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);
    nclist::overlaps_within(index, 15, 20, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsWithin, SimpleMaxGap) {
    std::vector<int> starts{ 10, 15 };
    std::vector<int> ends{ 20, 18 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsWithinParameters<int> params;
    nclist::OverlapsWithinWorkspace<int> workspace;
    std::vector<int> output;

    // Testing with default parameters as a control.
    nclist::overlaps_within(index, 10, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);
    nclist::overlaps_within(index, 15, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 1);

    params.max_gap = 2;
    nclist::overlaps_within(index, 10, 20, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);
    nclist::overlaps_within(index, 15, 20, params, workspace, output);
    EXPECT_TRUE(output.empty());

    // Still able to detect the overlap with the child, even though the parent is not counted.
    nclist::overlaps_within(index, 15, 18, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);
}

TEST(OverlapsWithin, SimpleOverlaps) {
    std::vector<int> starts{ 10, 50, 35, 40 };
    std::vector<int> ends  { 60, 95, 75, 77 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsWithinParameters<int> params;
    nclist::OverlapsWithinWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_within(index, 35, 40, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 2);

    nclist::overlaps_within(index, 55, 58, params, workspace, output);
    ASSERT_EQ(output.size(), 4);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);
    EXPECT_EQ(output[2], 2);
    EXPECT_EQ(output[3], 3);

    nclist::overlaps_within(index, 75, 77, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 1);
    EXPECT_EQ(output[1], 3);
}

TEST(OverlapsWithin, SimpleNested) {
    std::vector<int> starts{   0, 50, 60, 75,  0,  0, 10, 25 };
    std::vector<int> ends  { 100, 80, 70, 80, 30, 20, 20, 30 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsWithinParameters<int> params;
    nclist::OverlapsWithinWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_within(index, 75, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);
    EXPECT_EQ(output[2], 3);

    nclist::overlaps_within(index, 15, 18, params, workspace, output);
    ASSERT_EQ(output.size(), 4);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 4);
    EXPECT_EQ(output[2], 5);
    EXPECT_EQ(output[3], 6);

    nclist::overlaps_within(index, 20, 25, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 4);

    nclist::overlaps_within(index, 40, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);
}

/********************************************************************/

class OverlapsWithinReferenceTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int> > {
protected:
    void SetUp() {
        assemble(GetParam());
    }
};

TEST_P(OverlapsWithinReferenceTest, Basic) {
    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());

    nclist::OverlapsAnyWorkspace<int> a_work;
    nclist::OverlapsAnyParameters<int> a_params;
    std::vector<int> filtered;

    nclist::OverlapsWithinWorkspace<int> w_work;
    nclist::OverlapsWithinParameters<int> w_params;
    std::vector<int> results;

    for (int q = 0; q < nquery; ++q) {
        auto qstart = query_start[q];
        auto qend = query_end[q];

        nclist::overlaps_any(index, qstart, qend, a_params, a_work, results);
        filtered.clear();
        for (auto r : results) {
            if (qstart >= subject_start[r] && qend <= subject_end[r]) {
                filtered.push_back(r);
            }
        }
        std::sort(filtered.begin(), filtered.end());

        nclist::overlaps_within(index, query_start[q], query_end[q], w_params, w_work, results);
        std::sort(results.begin(), results.end());
        EXPECT_EQ(results, filtered);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapsWithin,
    OverlapsWithinReferenceTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000) // number of subject ranges
    )
);

class OverlapsWithinMinOverlapTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int, int> > {
protected:
    int min_overlap;
    void SetUp() {
        auto params = GetParam();
        assemble(params);
        min_overlap = std::get<2>(params);
    }
};

TEST_P(OverlapsWithinMinOverlapTest, Basic) {
    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());
    nclist::OverlapsWithinWorkspace<int> work;
    nclist::OverlapsWithinParameters<int> default_params, params;
    params.min_overlap = min_overlap;
    std::vector<int> results, default_results;

    for (int q = 0; q < nquery; ++q) {
        auto qstart = query_start[q];
        auto qend = query_end[q];

        nclist::overlaps_within(index, qstart, qend, params, work, results);
        if (qend - qstart < min_overlap) {
            EXPECT_TRUE(results.empty());
            continue;
        }

        nclist::overlaps_within(index, qstart, qend, default_params, work, default_results);
        std::sort(results.begin(), results.end());
        std::sort(default_results.begin(), default_results.end());
        EXPECT_EQ(results, default_results);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapsWithin,
    OverlapsWithinMinOverlapTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000), // number of subject ranges
        ::testing::Values(5, 20) // minimum overlap 
    )
);

class OverlapsWithinMaxGapTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int, int> > {
protected:
    int max_gap;
    void SetUp() {
        auto params = GetParam();
        assemble(params);
        max_gap = std::get<2>(params);
    }
};

TEST_P(OverlapsWithinMaxGapTest, Basic) {
    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());
    nclist::OverlapsWithinWorkspace<int> work;
    nclist::OverlapsWithinParameters<int> default_params, params;
    params.max_gap = max_gap;
    std::vector<int> results, filtered;

    for (int q = 0; q < nquery; ++q) {
        auto qstart = query_start[q];
        auto qend = query_end[q];
        auto qwidth = qend - qstart;

        nclist::overlaps_within(index, qstart, qend, default_params, work, results);
        filtered.clear();
        for (auto r : results) {
            if (subject_end[r] - subject_start[r] <= qwidth + max_gap) {
                filtered.push_back(r);
            }
        }
        std::sort(filtered.begin(), filtered.end());

        nclist::overlaps_within(index, query_start[q], query_end[q], params, work, results);
        std::sort(results.begin(), results.end());
        EXPECT_EQ(results, filtered);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapsWithin,
    OverlapsWithinMaxGapTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000), // number of subject ranges
        ::testing::Values(0, 10, 20) // maximum gap
    )
);

/********************************************************************/

TEST(OverlapsWithin, Double) {
    std::vector<double> test_starts { 52.2, 15.2, 12.1,  0.1 };
    std::vector<double> test_ends   { 58.1, 18.3, 80.2, 99.3 };

    auto index = nclist::build<int, double>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::OverlapsWithinWorkspace<int> workspace;
    nclist::OverlapsWithinParameters<double> params;
    std::vector<int> output;

    {
        nclist::overlaps_within(index, 100.0, 180.0, params, workspace, output);
        EXPECT_TRUE(output.empty());
    }

    {
        nclist::overlaps_within(index, 0.1, 60.0, params, workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 3);
    }

    {
        nclist::overlaps_within(index, 15.3, 55.0, params, workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 2);
        EXPECT_EQ(output[1], 3);
    }

    {
        nclist::overlaps_within(index, 55.0, 55.5, params, workspace, output);
        ASSERT_EQ(output.size(), 3);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 2);
        EXPECT_EQ(output[2], 3);
    }
}

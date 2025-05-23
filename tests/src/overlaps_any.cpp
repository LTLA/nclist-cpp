#include <gtest/gtest.h>

#include <vector>
#include <random>
#include <cstddef>

#include "nclist/overlaps_any.hpp"
#include "utils.hpp"

TEST(OverlapsAny, Empty) {
    auto index = nclist::build<int, int>(0, NULL, NULL);
    nclist::OverlapsAnyWorkspace<int> workspace;
    std::vector<int> output;
    nclist::overlaps_any(index, 100, 200, nclist::OverlapsAnyParameters<int>(), workspace, output);
    EXPECT_TRUE(output.empty());
}

/********************************************************************/

TEST(OverlapsAny, SimpleDisjoint) {
    std::vector<int> test_starts { 200, 300, 100, 500 };
    std::vector<int> test_ends { 280, 320, 170, 510 };

    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::OverlapsAnyWorkspace<int> workspace;
    std::vector<int> output;

    {
        nclist::overlaps_any(index, 50, 80, nclist::OverlapsAnyParameters<int>(), workspace, output);
        EXPECT_TRUE(output.empty());
    }

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

TEST(OverlapsAny, SimpleMaxGap) {
    std::vector<int> test_starts { 200, 300, 100, 500 };
    std::vector<int> test_ends { 280, 320, 170, 510 };

    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::OverlapsAnyWorkspace<int> workspace;
    std::vector<int> output, default_output;

    nclist::OverlapsAnyParameters<int> default_params;
    nclist::OverlapsAnyParameters<int> params;
    params.max_gap = 0;

    {
        nclist::overlaps_any(index, 90, 100, default_params, workspace, default_output);
        EXPECT_TRUE(default_output.empty());
        nclist::overlaps_any(index, 90, 100, params, workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::overlaps_any(index, 510, 520, default_params, workspace, default_output);
        EXPECT_TRUE(default_output.empty());
        nclist::overlaps_any(index, 510, 520, params, workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 3);
    }

    // Trying with a bigger max gap.
    params.max_gap = 10;
    {
        nclist::overlaps_any(index, 290, 290, default_params, workspace, default_output);
        EXPECT_TRUE(default_output.empty());
        nclist::overlaps_any(index, 290, 290, params, workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
    }

    params.max_gap = 100;
    {
        nclist::overlaps_any(index, 300, 400, default_params, workspace, default_output);
        ASSERT_EQ(default_output.size(), 1);
        EXPECT_EQ(default_output[0], 1);
        nclist::overlaps_any(index, 300, 400, params, workspace, output);
        ASSERT_EQ(output.size(), 3);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 3);
    }
}

TEST(OverlapsAny, SimpleMinOverlap) {
    std::vector<int> test_starts { 200, 300, 100, 500 };
    std::vector<int> test_ends { 280, 320, 170, 510 };

    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::OverlapsAnyWorkspace<int> workspace;
    std::vector<int> output, default_output;

    nclist::OverlapsAnyParameters<int> default_params;
    nclist::OverlapsAnyParameters<int> params;
    params.min_overlap = 10;

    {
        nclist::overlaps_any(index, 100, 105, default_params, workspace, default_output);
        ASSERT_EQ(default_output.size(), 1);
        EXPECT_EQ(default_output[0], 2);
        nclist::overlaps_any(index, 100, 105, params, workspace, output);
        EXPECT_TRUE(output.empty());
    }

    {
        nclist::overlaps_any(index, 275, 310, default_params, workspace, default_output);
        ASSERT_EQ(default_output.size(), 2);
        std::sort(default_output.begin(), default_output.end());
        EXPECT_EQ(default_output[0], 0);
        EXPECT_EQ(default_output[1], 1);
        nclist::overlaps_any(index, 275, 310, params, workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 1);
    }

    // Skipping if the input range is too small.
    {
        nclist::overlaps_any(index, 290, 295, params, workspace, output);
        EXPECT_TRUE(output.empty());
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

/********************************************************************/

class OverlapsAnyReferenceTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int> > {
protected:
    void SetUp() {
        assemble(GetParam());
    }
};

TEST_P(OverlapsAnyReferenceTest, Basic) {
    std::vector<std::vector<int> > ref; 
    reference_search(query_start, query_end, subject_start, subject_end, ref);

    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());
    nclist::OverlapsAnyWorkspace<int> work;
    nclist::OverlapsAnyParameters<int> params;
    std::vector<int> results;

    for (int q = 0; q < nquery; ++q) {
        nclist::overlaps_any(index, query_start[q], query_end[q], params, work, results);
        std::sort(results.begin(), results.end());
        EXPECT_EQ(results, ref[q]);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapsAny,
    OverlapsAnyReferenceTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000) // number of subject ranges
    )
);

class OverlapsAnyMinOverlapTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int, int> > {
protected:
    int min_overlap;
    void SetUp() {
        auto params = GetParam();
        assemble(params);
        min_overlap = std::get<2>(params);
    }
};

TEST_P(OverlapsAnyMinOverlapTest, Basic) {
    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());
    nclist::OverlapsAnyWorkspace<int> work;
    nclist::OverlapsAnyParameters<int> default_params, params;
    params.min_overlap = min_overlap;
    std::vector<int> results, filtered;

    for (int q = 0; q < nquery; ++q) {
        nclist::overlaps_any(index, query_start[q], query_end[q], default_params, work, results);
        filtered.clear();
        for (auto r : results) {
            if (min_overlap <= std::min(query_end[q], subject_end[r]) - std::max(query_start[q], subject_start[r])) {
                filtered.push_back(r);
            }
        }
        std::sort(filtered.begin(), filtered.end());

        nclist::overlaps_any(index, query_start[q], query_end[q], params, work, results);
        std::sort(results.begin(), results.end());
        EXPECT_EQ(results, filtered);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapsAny,
    OverlapsAnyMinOverlapTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000), // number of subject ranges
        ::testing::Values(5, 20) // minimum overlap 
    )
);

class OverlapsAnyMaxGapTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int, int> > {
protected:
    int max_gap;
    void SetUp() {
        auto params = GetParam();
        assemble(params);
        max_gap = std::get<2>(params);
    }
};

TEST_P(OverlapsAnyMaxGapTest, Basic) {
    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());
    nclist::OverlapsAnyWorkspace<int> work;
    nclist::OverlapsAnyParameters<int> default_params, params;
    params.max_gap = max_gap;
    std::vector<int> results, default_results;

    for (int q = 0; q < nquery; ++q) {
        // Pushing it out by an extra 1 so that the query start/end boundaries are now inclusive when comparing to subject end/start coordinates, respectively.
        nclist::overlaps_any(index, query_start[q] - max_gap - 1, query_end[q] + max_gap + 1, default_params, work, default_results);
        std::sort(default_results.begin(), default_results.end());

        nclist::overlaps_any(index, query_start[q], query_end[q], params, work, results);
        std::sort(results.begin(), results.end());
        EXPECT_EQ(results, default_results);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapsAny,
    OverlapsAnyMaxGapTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000), // number of subject ranges
        ::testing::Values(0, 10) // maximum gap
    )
);

/********************************************************************/

TEST(OverlapsAny, Unsigned) {
    std::vector<unsigned> test_starts { 200, 300, 100, 500 };
    std::vector<unsigned> test_ends { 280, 320, 170, 510 };

    auto index = nclist::build<std::size_t, unsigned>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::OverlapsAnyWorkspace<std::size_t> workspace;
    std::vector<std::size_t> output;

    {
        nclist::overlaps_any(index, 50u, 80u, nclist::OverlapsAnyParameters<unsigned>(), workspace, output);
        EXPECT_TRUE(output.empty());
    }

    {
        nclist::overlaps_any(index, 150u, 200u, nclist::OverlapsAnyParameters<unsigned>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::overlaps_any(index, 150u, 300u, nclist::OverlapsAnyParameters<unsigned>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 2);
    }

    {
        nclist::overlaps_any(index, 210u, 310u, nclist::OverlapsAnyParameters<unsigned>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
    }

    // Behaves with a max gap.
    {
        nclist::OverlapsAnyParameters<unsigned> params;
        params.max_gap = 100; // check that we avoid underflow when defining the search start.
        nclist::overlaps_any(index, 90u, 200u, params, workspace, output);
        ASSERT_EQ(output.size(), 3);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 2);
    }

    // Behaves with a minimum overlap.
    {
        nclist::OverlapsAnyParameters<unsigned> params;
        params.min_overlap = std::numeric_limits<unsigned>::max(); // check that we return early if a huge overlap is specified.
        nclist::overlaps_any(index, 90u, 200u, params, workspace, output);
        EXPECT_TRUE(output.empty());
    }
}

TEST(OverlapsAny, Double) {
    std::vector<double> test_starts { 200.5, 300.1, 100.8, 500.5 };
    std::vector<double> test_ends { 280.2, 320.9, 170.1, 510.5 };

    auto index = nclist::build<int, double>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::OverlapsAnyWorkspace<int> workspace;
    std::vector<int> output;

    {
        nclist::overlaps_any(index, 50.0, 80.0, nclist::OverlapsAnyParameters<double>(), workspace, output);
        EXPECT_TRUE(output.empty());
    }

    {
        nclist::overlaps_any(index, 150.0, 200.5, nclist::OverlapsAnyParameters<double>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::overlaps_any(index, 150.0, 250.0, nclist::OverlapsAnyParameters<double>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 2);
    }

    {
        nclist::overlaps_any(index, 170.1, 320.0, nclist::OverlapsAnyParameters<double>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
    }
}

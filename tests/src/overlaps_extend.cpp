#include <gtest/gtest.h>

#include <vector>
#include <random>

#include "nclist/overlaps_extend.hpp"
#include "nclist/overlaps_any.hpp"
#include "utils.hpp"

TEST(OverlapsExtend, Empty) {
    auto index = nclist::build<int, int>(0, NULL, NULL);
    nclist::OverlapsExtendWorkspace<int> workspace;
    std::vector<int> output;
    nclist::overlaps_extend(index, 100, 200, nclist::OverlapsExtendParameters<int>(), workspace, output);
    EXPECT_TRUE(output.empty());
}

/********************************************************************/

TEST(OverlapsExtend, SimpleDisjoint) {
    std::vector<int> starts{ 100, 50, 230, 180, 20 };
    std::vector<int> ends  { 150, 80, 250, 200, 30 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsExtendParameters<int> params;
    nclist::OverlapsExtendWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_extend(index, 40, 90, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);

    nclist::overlaps_extend(index, 10, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 1);
    EXPECT_EQ(output[1], 4);

    nclist::overlaps_extend(index, 100, 300, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 2);
    EXPECT_EQ(output[2], 3);

    nclist::overlaps_extend(index, 0, 25, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsExtend, SimpleMaxGap) {
    std::vector<int> starts{ 100, 50, 230, 180, 20 };
    std::vector<int> ends  { 150, 80, 250, 200, 30 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsExtendParameters<int> params;
    nclist::OverlapsExtendWorkspace<int> workspace;
    std::vector<int> output;

    params.max_gap = 20;
    nclist::overlaps_extend(index, 40, 90, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);
    params.max_gap = 0;
    nclist::overlaps_extend(index, 40, 90, params, workspace, output);
    EXPECT_TRUE(output.empty());

    params.max_gap = 50;
    nclist::overlaps_extend(index, 20, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 1);
    EXPECT_EQ(output[1], 4);
    params.max_gap = 30;
    nclist::overlaps_extend(index, 20, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);

    params.max_gap = 0;
    nclist::overlaps_extend(index, 230, 250, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 2);
}

TEST(OverlapsExtend, SimpleMinOverlap) {
    std::vector<int> starts{ 30, 50, 60, 10,  0 };
    std::vector<int> ends  { 40, 80, 70, 25, 55 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsExtendParameters<int> params;
    nclist::OverlapsExtendWorkspace<int> workspace;
    std::vector<int> output;

    params.min_overlap = 20;
    nclist::overlaps_extend(index, 30, 40, params, workspace, output);
    EXPECT_TRUE(output.empty());

    params.min_overlap = 20;
    nclist::overlaps_extend(index, 30, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);
    params.min_overlap = 10;
    nclist::overlaps_extend(index, 30, 80, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);
    EXPECT_EQ(output[2], 2);

    params.min_overlap = 20;
    nclist::overlaps_extend(index, 0, 100, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 1);
    EXPECT_EQ(output[1], 4);
}

TEST(OverlapsExtend, SimpleOverlaps) {
    std::vector<int> starts{ 100,  80, 130,  50, 150 };
    std::vector<int> ends  { 150, 110, 200, 100, 250 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsExtendParameters<int> params;
    nclist::OverlapsExtendWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_extend(index, 40, 100, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 3);

    nclist::overlaps_extend(index, 80, 200, params, workspace, output);
    ASSERT_EQ(output.size(), 3);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);
    EXPECT_EQ(output[2], 2);

    nclist::overlaps_extend(index, 120, 300, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 2);
    EXPECT_EQ(output[1], 4);
}

TEST(OverlapsExtend, SimpleNested) {
    std::vector<int> starts{ 100,   0, 130, 20, 40, 70, 50 };
    std::vector<int> ends  { 150, 200, 140, 80, 60, 80, 55 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsExtendParameters<int> params;
    nclist::OverlapsExtendWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_extend(index, 50, 60, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 6);

    nclist::overlaps_extend(index, 30, 70, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 4);
    EXPECT_EQ(output[1], 6);

    nclist::overlaps_extend(index, 50, 180, params, workspace, output);
    ASSERT_EQ(output.size(), 4);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 2);
    EXPECT_EQ(output[2], 5);
    EXPECT_EQ(output[3], 6);

    nclist::overlaps_extend(index, 0, 200, params, workspace, output);
    ASSERT_EQ(output.size(), 7);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);
    EXPECT_EQ(output[2], 2);
    EXPECT_EQ(output[3], 3);
    EXPECT_EQ(output[4], 4);
    EXPECT_EQ(output[5], 5);
    EXPECT_EQ(output[6], 6);
}

/********************************************************************/

class OverlapsExtendReferenceTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int> > {
protected:
    void SetUp() {
        assemble(GetParam());
    }
};

TEST_P(OverlapsExtendReferenceTest, Basic) {
    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());

    nclist::OverlapsAnyWorkspace<int> a_work;
    nclist::OverlapsAnyParameters<int> a_params;
    std::vector<int> filtered;

    nclist::OverlapsExtendWorkspace<int> w_work;
    nclist::OverlapsExtendParameters<int> w_params;
    std::vector<int> results;

    for (int q = 0; q < nquery; ++q) {
        auto qstart = query_start[q];
        auto qend = query_end[q];

        nclist::overlaps_any(index, qstart, qend, a_params, a_work, results);
        filtered.clear();
        for (auto r : results) {
            if (qstart <= subject_start[r] && qend >= subject_end[r]) {
                filtered.push_back(r);
            }
        }
        std::sort(filtered.begin(), filtered.end());

        nclist::overlaps_extend(index, query_start[q], query_end[q], w_params, w_work, results);
        std::sort(results.begin(), results.end());
        EXPECT_EQ(results, filtered);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapsExtend,
    OverlapsExtendReferenceTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000) // number of subject ranges
    )
);

class OverlapsExtendMinOverlapTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int, int> > {
protected:
    int min_overlap;
    void SetUp() {
        auto params = GetParam();
        assemble(params);
        min_overlap = std::get<2>(params);
    }
};

TEST_P(OverlapsExtendMinOverlapTest, Basic) {
    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());
    nclist::OverlapsExtendWorkspace<int> work;
    nclist::OverlapsExtendParameters<int> default_params, params;
    params.min_overlap = min_overlap;
    std::vector<int> results, filtered;

    for (int q = 0; q < nquery; ++q) {
        auto qstart = query_start[q];
        auto qend = query_end[q];

        nclist::overlaps_extend(index, qstart, qend, default_params, work, results);
        filtered.clear();
        for (auto r : results) {
            if (subject_end[r] - subject_start[r] >= min_overlap) {
                filtered.push_back(r);
            }
        }
        std::sort(filtered.begin(), filtered.end());

        nclist::overlaps_extend(index, qstart, qend, params, work, results);
        std::sort(results.begin(), results.end());
        EXPECT_EQ(results, filtered);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapsExtend,
    OverlapsExtendMinOverlapTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000), // number of subject ranges
        ::testing::Values(5, 20) // minimum overlap 
    )
);

class OverlapsExtendMaxGapTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int, int> > {
protected:
    int max_gap;
    void SetUp() {
        auto params = GetParam();
        assemble(params);
        max_gap = std::get<2>(params);
    }
};

TEST_P(OverlapsExtendMaxGapTest, Basic) {
    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());
    nclist::OverlapsExtendWorkspace<int> work;
    nclist::OverlapsExtendParameters<int> default_params, params;
    params.max_gap = max_gap;
    std::vector<int> results, filtered;

    for (int q = 0; q < nquery; ++q) {
        auto qstart = query_start[q];
        auto qend = query_end[q];
        auto qwidth = qend - qstart;

        nclist::overlaps_extend(index, qstart, qend, default_params, work, results);
        filtered.clear();
        for (auto r : results) {
            if (qwidth <= subject_end[r] - subject_start[r] + max_gap) {
                filtered.push_back(r);
            }
        }
        std::sort(filtered.begin(), filtered.end());

        nclist::overlaps_extend(index, query_start[q], query_end[q], params, work, results);
        std::sort(results.begin(), results.end());
        EXPECT_EQ(results, filtered);
    }
}

INSTANTIATE_TEST_SUITE_P(
    OverlapsExtend,
    OverlapsExtendMaxGapTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000), // number of subject ranges
        ::testing::Values(0, 10, 20) // maximum gap
    )
);

/********************************************************************/

TEST(OverlapsExtend, Double) {
    std::vector<double> test_starts { 28.3,  2.3,  0.1, 45.2, 10.1 };
    std::vector<double> test_ends   { 37.5, 19.3, 40.2, 49.7, 50.3 };

    auto index = nclist::build<int, double>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::OverlapsExtendWorkspace<int> workspace;
    nclist::OverlapsExtendParameters<double> params;
    std::vector<int> output;

    nclist::overlaps_extend(index, 0.0, 10.0, params, workspace, output);
    EXPECT_TRUE(output.empty());

    nclist::overlaps_extend(index, 40.0, 50.0, params, workspace, output);
    EXPECT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 3);

    nclist::overlaps_extend(index, 0.1, 50.0, params, workspace, output);
    ASSERT_EQ(output.size(), 4);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);
    EXPECT_EQ(output[2], 2);
    EXPECT_EQ(output[3], 3);

    nclist::overlaps_extend(index, 20.3, 50.3, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 3);
}

/********************************************************************/

TEST(OverlapsExtend, Duplicates) {
    std::vector<int> starts{ 100, 50, 230, 50, 180, 20, 20 };
    std::vector<int> ends  { 150, 80, 250, 80, 200, 30, 30 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsExtendParameters<int> params;
    nclist::OverlapsExtendWorkspace<int> workspace;
    std::vector<int> output;

    nclist::overlaps_extend(index, 40, 90, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 1);
    EXPECT_EQ(output[1], 3);

    nclist::overlaps_extend(index, 10, 90, params, workspace, output);
    ASSERT_EQ(output.size(), 4);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 1);
    EXPECT_EQ(output[1], 3);
    EXPECT_EQ(output[2], 5);
    EXPECT_EQ(output[3], 6);

    nclist::overlaps_extend(index, 25, 200, params, workspace, output);
    ASSERT_EQ(output.size(), 4);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);
    EXPECT_EQ(output[2], 3);
    EXPECT_EQ(output[3], 4);

    nclist::overlaps_extend(index, 5, 20, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsExtend, EarlyQuit) {
    std::vector<int> starts{ 100, 50, 230, 180, 20 };
    std::vector<int> ends  { 150, 80, 250, 200, 30 };
    auto index = nclist::build<int, int>(starts.size(), starts.data(), ends.data());

    nclist::OverlapsExtendWorkspace<int> workspace;
    nclist::OverlapsExtendParameters<int> params;
    params.quit_on_first = true;
    std::vector<int> output;

    nclist::overlaps_extend(index, 0, 100, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_TRUE(output[0] == 1 || output[0] == 4);

    nclist::overlaps_extend(index, 90, 240, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_TRUE(output[0] == 0 || output[0] == 3);

    nclist::overlaps_extend(index, 240, 300, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

TEST(OverlapsExtend, ZeroWidth) {
    std::vector<int> test_starts { 200, 400 };
    std::vector<int> test_ends { 200, 500 };
    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::OverlapsExtendWorkspace<int> workspace;
    nclist::OverlapsExtendParameters<int> params;
    std::vector<int> output;

    nclist::overlaps_extend(index, 0, 200, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);
    nclist::overlaps_extend(index, 100, 300, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);
    nclist::overlaps_extend(index, 200, 300, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);

    nclist::overlaps_extend(index, 200, 200, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);

    params.min_overlap = 10;
    nclist::overlaps_extend(index, 100, 300, params, workspace, output);
    EXPECT_TRUE(output.empty());
}

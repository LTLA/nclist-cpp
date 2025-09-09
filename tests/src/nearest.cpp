#include <gtest/gtest.h>

#include <vector>
#include <random>
#include <cstddef>

#include "nclist/nearest.hpp"
#include "nclist/overlaps_any.hpp"
#include "utils.hpp"

TEST(Nearest, Empty) {
    auto index = nclist::build<int, int>(0, NULL, NULL);
    nclist::NearestWorkspace<int> workspace;
    std::vector<int> output;
    nclist::nearest(index, 100, 200, nclist::NearestParameters<int>(), workspace, output);
    EXPECT_TRUE(output.empty());
}

/********************************************************************/

TEST(Nearest, SimpleDisjoint) {
    std::vector<int> test_starts { 200, 300, 100, 500 };
    std::vector<int> test_ends { 280, 320, 170, 510 };

    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::NearestWorkspace<int> workspace;
    std::vector<int> output;

    {
        nclist::nearest(index, 50, 80, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::nearest(index, 520, 600, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 3);
    }

    {
        nclist::nearest(index, 180, 190, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 2);
    }

    // Now testing all the overlaps.
    {
        nclist::nearest(index, 150, 200, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::nearest(index, 150, 300, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 2);
    }

    {
        nclist::nearest(index, 210, 310, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
    }

    {
        nclist::nearest(index, 90, 600, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 4);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 2);
        EXPECT_EQ(output[3], 3);
    }
}

TEST(Nearest, SimpleOverlaps) {
    std::vector<int> test_starts { 200, 300, 100, 500 };
    std::vector<int> test_ends { 600, 720, 510, 1000 };

    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::NearestWorkspace<int> workspace;
    std::vector<int> output;

    {
        nclist::nearest(index, 50, 80, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::nearest(index, 1020, 1100, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 3);
    }

    // Alright, trying all the overlaps.
    {
        nclist::nearest(index, 150, 200, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::nearest(index, 50, 400, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 3);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 2);
    }

    {
        nclist::nearest(index, 700, 1000, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 1);
        EXPECT_EQ(output[1], 3);
    }

    {
        nclist::nearest(index, 500, 600, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 4);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 2);
        EXPECT_EQ(output[3], 3);
    }
}

TEST(Nearest, SimpleNested) {
    std::vector<int> test_starts { 0, 20, 20, 40, 70, 90 };
    std::vector<int> test_ends { 100, 60, 30, 50, 95, 95 };

    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::NearestWorkspace<int> workspace;
    std::vector<int> output;

    {
        nclist::nearest(index, -10, 0, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 0);
    }

    {
        nclist::nearest(index, 100, 110, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 0);
    }

    // Alright, trying all the overlaps.
    {
        nclist::nearest(index, 0, 10, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 0);
    }

    {
        nclist::nearest(index, 42, 45, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 3);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 3);
    }

    {
        nclist::nearest(index, 35, 40, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
    }

    {
        nclist::nearest(index, 45, 80, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 4);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 3);
        EXPECT_EQ(output[3], 4);
    }
}

TEST(Nearest, NestedFlush) {
    std::vector<int> test_starts { 0, 20, 40, 30, 80,  85, 80,  80 };
    std::vector<int> test_ends {  50, 50, 50, 40, 90, 100, 95, 100 };

    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::NearestWorkspace<int> workspace;
    std::vector<int> output;

    {
        nclist::nearest(index, 55, 65, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 3);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 2);
    }

    {
        nclist::nearest(index, 70, 80, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 3);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 4);
        EXPECT_EQ(output[1], 6);
        EXPECT_EQ(output[2], 7);
    }

    {
        nclist::nearest(index, 50, 80, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 6);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 2);
        EXPECT_EQ(output[3], 4);
        EXPECT_EQ(output[4], 6);
        EXPECT_EQ(output[5], 7);
    }

    // Alright, trying an overlap.
    {
        nclist::nearest(index, 70, 90, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 4);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 4);
        EXPECT_EQ(output[1], 5);
        EXPECT_EQ(output[2], 6);
        EXPECT_EQ(output[3], 7);
    }

    {
        nclist::nearest(index, 35, 50, nclist::NearestParameters<int>(), workspace, output);
        ASSERT_EQ(output.size(), 4);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
        EXPECT_EQ(output[2], 2);
        EXPECT_EQ(output[3], 3);
    }
}

/********************************************************************/

class NearestReferenceTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int> > {
protected:
    void SetUp() {
        assemble(GetParam());
    }
};

TEST_P(NearestReferenceTest, Basic) {
    auto index = nclist::build(nsubject, subject_start.data(), subject_end.data());
    nclist::NearestWorkspace<int> work;
    nclist::NearestParameters<int> params;
    std::vector<int> results;

    nclist::OverlapsAnyWorkspace<int> owork;
    nclist::OverlapsAnyParameters<int> oparams;
    std::vector<int> ref_results;

    const int nsubjects = subject_end.size();
    std::vector<std::pair<int, int> > all_ends, all_starts;
    all_starts.reserve(nsubjects);
    all_ends.reserve(nsubjects);
    for (int i = 0; i < nsubjects; ++i) {
        all_starts.emplace_back(subject_start[i], i);
        all_ends.emplace_back(subject_end[i], i);
    }
    std::sort(all_starts.begin(), all_starts.end());
    std::sort(all_ends.begin(), all_ends.end());

    for (int q = 0; q < nquery; ++q) {
        const auto qs = query_start[q];
        const auto qe = query_end[q];
        nclist::nearest(index, qs, qe, params, work, results);

        nclist::overlaps_any(index, qs, qe, oparams, owork, ref_results);
        if (!ref_results.empty()) {
            std::sort(ref_results.begin(), ref_results.end());
            std::sort(results.begin(), results.end());
            EXPECT_EQ(results, ref_results);
            continue;
        }

        ref_results.clear();

        std::optional<int> to_previous, to_next; 
        auto first_at = std::upper_bound(all_ends.begin(), all_ends.end(), std::make_pair(qs, nsubjects)) - all_ends.begin();
        if (first_at != 0) {
            to_previous = qs - all_ends[first_at - 1].first;
        }
        auto last_at = std::lower_bound(all_starts.begin(), all_starts.end(), std::make_pair(qe, 0)) - all_starts.begin();
        if (last_at != nsubjects) {
            to_next = all_starts[last_at].first - qe;
        }

        if (!to_next.has_value() || (to_previous.has_value() && *to_previous <= *to_next)) {
            const auto limit = all_ends[first_at - 1].first;
            while (first_at > 0) {
                --first_at;
                if (all_ends[first_at].first < limit) {
                    break;
                }
                ref_results.push_back(all_ends[first_at].second);
            }
        }

        if (!to_previous.has_value() || (to_next.has_value() && *to_next <= *to_previous)) {
            const auto limit = all_starts[last_at].first;
            while (last_at < nsubjects) {
                if (all_starts[last_at].first > limit) {
                    break;
                }
                ref_results.push_back(all_starts[last_at].second);
                ++last_at;
            }
        }

        std::sort(ref_results.begin(), ref_results.end());
        std::sort(results.begin(), results.end());
        EXPECT_EQ(results, ref_results);
    }
}

INSTANTIATE_TEST_SUITE_P(
    Nearest,
    NearestReferenceTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000) // number of subject ranges
    )
);

/********************************************************************/

TEST(Nearest, Unsigned) {
    std::vector<unsigned> test_starts { 200, 300, 100, 500 };
    std::vector<unsigned> test_ends { 280, 320, 170, 510 };

    auto index = nclist::build<std::size_t, unsigned>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::NearestWorkspace<std::size_t> workspace;
    std::vector<std::size_t> output;

    {
        nclist::nearest(index, 50u, 80u, nclist::NearestParameters<unsigned>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::nearest(index, 330u, 400u, nclist::NearestParameters<unsigned>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 1);
    }

    {
        nclist::nearest(index, 180u, 190u, nclist::NearestParameters<unsigned>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 2);
    }

    // Now with overlaps.
    {
        nclist::nearest(index, 150u, 200u, nclist::NearestParameters<unsigned>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::nearest(index, 150u, 300u, nclist::NearestParameters<unsigned>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 2);
    }

    {
        nclist::nearest(index, 210u, 310u, nclist::NearestParameters<unsigned>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
    }
}

TEST(Nearest, Double) {
    std::vector<double> test_starts { 200.5, 300.1, 100.8, 500.5 };
    std::vector<double> test_ends { 280.2, 320.9, 170.1, 510.5 };

    auto index = nclist::build<int, double>(test_starts.size(), test_starts.data(), test_ends.data());
    nclist::NearestWorkspace<int> workspace;
    std::vector<int> output;

    {
        nclist::nearest(index, 50.0, 80.0, nclist::NearestParameters<double>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::nearest(index, 280.5, 290.0, nclist::NearestParameters<double>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 0);
    }

    // Now with overlaps.
    {
        nclist::nearest(index, 150.0, 200.5, nclist::NearestParameters<double>(), workspace, output);
        ASSERT_EQ(output.size(), 1);
        EXPECT_EQ(output[0], 2);
    }

    {
        nclist::nearest(index, 150.0, 250.0, nclist::NearestParameters<double>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 2);
    }

    {
        nclist::nearest(index, 170.1, 320.0, nclist::NearestParameters<double>(), workspace, output);
        ASSERT_EQ(output.size(), 2);
        std::sort(output.begin(), output.end());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 1);
    }
}

/********************************************************************/

TEST(Nearest, Duplicates) {
    std::vector<int> test_starts { 200, 200, 300, 100, 500, 100 };
    std::vector<int> test_ends { 280, 280, 320, 170, 510, 170 };
    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::NearestWorkspace<int> workspace;
    nclist::NearestParameters<int> params;
    std::vector<int> output;

    nclist::nearest(index, 280, 290, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);

    nclist::nearest(index, 175, 190, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 3);
    EXPECT_EQ(output[1], 5);

    nclist::nearest(index, 180, 190, params, workspace, output);
    ASSERT_EQ(output.size(), 4);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);
    EXPECT_EQ(output[2], 3);
    EXPECT_EQ(output[3], 5);

    // Testing overlaps. 
    nclist::nearest(index, 250, 300, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);

    nclist::nearest(index, 150, 250, params, workspace, output);
    ASSERT_EQ(output.size(), 4);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);
    EXPECT_EQ(output[2], 3);
    EXPECT_EQ(output[3], 5);

    nclist::nearest(index, 250, 550, params, workspace, output);
    ASSERT_EQ(output.size(), 4);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);
    EXPECT_EQ(output[2], 2);
    EXPECT_EQ(output[3], 4);
}

TEST(Nearest, EarlyQuit) {
    // Add some nesting to check that we can still quit early when we walk down the tree outside of the overlaps.
    std::vector<int> test_starts{ 200, 300, 200, 100, 500, 150 };
    std::vector<int> test_ends  { 280, 320, 250, 170, 510, 170 };
    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::NearestWorkspace<int> workspace;
    nclist::NearestParameters<int> params;
    params.quit_on_first = true;
    std::vector<int> output;

    nclist::nearest(index, 190, 195, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_TRUE(output[0] == 0 || output[0] == 2);

    nclist::nearest(index, 175, 190, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_TRUE(output[0] == 3 || output[0] == 5);

    nclist::nearest(index, 170, 200, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_TRUE(output[0] == 0 || output[0] == 2 || output[0] == 3 || output[0] == 5);

    nclist::nearest(index, 280, 300, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_TRUE(output[0] == 0 || output[0] == 1);

    // Just checking this for some more coverage of the early quit where there's only one nearest range.
    nclist::nearest(index, 330, 350, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);

    nclist::nearest(index, 450, 485, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 4);

    // Now with overlaps.
    nclist::nearest(index, 250, 350, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_TRUE(output[0] == 0 || output[0] == 1);

    nclist::nearest(index, 300, 550, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_TRUE(output[0] == 1 || output[0] == 4);
}

TEST(Nearest, ZeroWidth) {
    std::vector<int> test_starts { 200, 400 };
    std::vector<int> test_ends { 200, 500 };
    auto index = nclist::build<int, int>(test_starts.size(), test_starts.data(), test_ends.data());

    nclist::NearestWorkspace<int> workspace;
    nclist::NearestParameters<int> params;
    std::vector<int> output;

    // Zero-length ranges are kind of weird as they don't overlap even with themselves.
    // But, they can still contribute to the detection of the nearest range as we just care about the gap.
    nclist::nearest(index, 200, 300, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);

    nclist::nearest(index, 500, 500, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 1);

    nclist::nearest(index, 200, 200, params, workspace, output);
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);

    nclist::nearest(index, 200, 400, params, workspace, output);
    ASSERT_EQ(output.size(), 2);
    std::sort(output.begin(), output.end());
    EXPECT_EQ(output[0], 0);
    EXPECT_EQ(output[1], 1);

    nclist::nearest(index, 199, 400, params, workspace, output); // enclosing the first subject, so now we have an overlap.
    ASSERT_EQ(output.size(), 1);
    EXPECT_EQ(output[0], 0);
}

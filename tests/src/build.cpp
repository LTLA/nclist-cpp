#include <gtest/gtest.h>

#include <vector>
#include <random>
#include <cstddef>
#include <cstdint>

#include "nclist/build.hpp"
#include "nclist/overlaps_any.hpp"
#include "utils.hpp"

class BuildTest : public OverlapsTestCore, public ::testing::TestWithParam<std::tuple<int, int> > {
protected:
    void SetUp() {
        assemble(GetParam());
    }
};

class Incrementer {
public:
    Incrementer(const int* ptr) : my_ptr(ptr) {}
    int operator[](int i) const {
        return my_ptr[i] + 1;
    }
private:
    const int* my_ptr;
};

TEST_P(BuildTest, Basic) {
    std::vector<int> keep;
    keep.reserve(nsubject / 2);

    // Checking subset.
    {
        std::vector<int> subset_subject_start;
        subset_subject_start.reserve(keep.size());
        std::vector<int> subset_subject_end;
        subset_subject_end.reserve(keep.size());

        for (int i = 0; i < nsubject; ++i) {
            if (i % 2) {
                keep.push_back(i);
                subset_subject_start.push_back(subject_start[i]);
                subset_subject_end.push_back(subject_end[i]);
            }
        }

        auto sub_index = nclist::build<int>(keep.size(), keep.data(), subject_start.data(), subject_end.data());
        auto ref_index = nclist::build<int>(keep.size(), subset_subject_start.data(), subset_subject_end.data());

        nclist::OverlapsAnyWorkspace<int> work;
        nclist::OverlapsAnyParameters<int> params;
        std::vector<int> ref_results, sub_results;

        for (int q = 0; q < nquery; ++q) {
            nclist::overlaps_any(ref_index, query_start[q], query_end[q], params, work, ref_results);
            nclist::overlaps_any(sub_index, query_start[q], query_end[q], params, work, sub_results);
            for (auto& i : ref_results) {
                i = keep[i];
            }
            EXPECT_EQ(sub_results, ref_results);
        }
    }

    // Checking custom arrays.
    auto inc_subject_end = subject_end;
    for (auto& s : inc_subject_end) {
        ++s;
    }

    {
        auto ref_index = nclist::build(nsubject, subject_start.data(), inc_subject_end.data());
        auto inc_index = nclist::build_custom(nsubject, subject_start.data(), Incrementer(subject_end.data()));

        nclist::OverlapsAnyWorkspace<int> work;
        nclist::OverlapsAnyParameters<int> params;
        std::vector<int> ref_results, inc_results;

        for (int q = 0; q < nquery; ++q) {
            nclist::overlaps_any(ref_index, query_start[q], query_end[q], params, work, ref_results);
            nclist::overlaps_any(inc_index, query_start[q], query_end[q], params, work, inc_results);
            EXPECT_EQ(inc_results, ref_results);
        }
    }

    // Checking custom arrays with subsetting.
    {
        auto ref_index = nclist::build<int>(keep.size(), keep.data(), subject_start.data(), inc_subject_end.data());
        auto inc_index = nclist::build_custom<int>(keep.size(), keep.data(), subject_start.data(), Incrementer(subject_end.data()));

        nclist::OverlapsAnyWorkspace<int> work;
        nclist::OverlapsAnyParameters<int> params;
        std::vector<int> ref_results, inc_results;

        for (int q = 0; q < nquery; ++q) {
            nclist::overlaps_any(ref_index, query_start[q], query_end[q], params, work, ref_results);
            nclist::overlaps_any(inc_index, query_start[q], query_end[q], params, work, inc_results);
            EXPECT_EQ(inc_results, ref_results);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    Build,
    BuildTest,
    ::testing::Combine(
        ::testing::Values(10, 100, 1000), // num of query ranges
        ::testing::Values(10, 100, 1000)  // number of subject ranges
    )
);

TEST(Build, SafeResize) {
    struct MockVector {
        MockVector(std::uint8_t s) : s(s) {}
        std::uint8_t size() const {
            return s;
        }
        void resize(std::uint8_t new_s) {
            s = new_s;
        }
        std::uint8_t s;
    };

    MockVector y(0);
    nclist::safe_resize(y, 10);
    EXPECT_EQ(y.size(), 10);

    std::string msg;
    try {
        nclist::safe_resize(y, 1000);
    } catch (std::exception& e) {
        msg = e.what();
    }
    EXPECT_TRUE(msg.find("resize") != std::string::npos);
}

struct MockIterator {
    MockIterator(std::size_t pos) : pos(pos) {}
    std::size_t pos;
};

std::uint8_t operator-(const MockIterator& left, const MockIterator& right) {
    return right.pos - left.pos;
}

TEST(Build, CheckSafePtrdiff) {
    nclist::check_safe_ptrdiff<MockIterator>(static_cast<std::int8_t>(10));
    nclist::check_safe_ptrdiff<MockIterator>(10);

    std::string msg;
    try {
        nclist::check_safe_ptrdiff<MockIterator>(1000);
    } catch (std::exception& e) {
        msg = e.what();
    }
    EXPECT_TRUE(msg.find("iterator subtraction") != std::string::npos);
}

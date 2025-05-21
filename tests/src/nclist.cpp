#include <gtest/gtest.h>

#include <vector>

#include "nclist/nclist.hpp"

TEST(Nclist, Basic) {
    std::vector<int> test{1,2,3};
    auto output = nclist::build(test.size(), test.data(), test.data());
    EXPECT_EQ(output.nodes.size(), 4);
}

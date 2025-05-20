#ifndef NCLIST_HPP
#define NCLIST_HPP

#include <stdexcept>
#include <vector>
#include <cstddef>

namespace nclist {

/**
 * @cond
 */
template<typename Index_>
struct Child {
    Child() = default;
    Child(Index_ offset, Index_ id) : offset(offset), id(id) {}
    Index_ offset = 0;
    Index_ id = 0;
};

template<typename Index_>
struct Node {
    std::vector<Child<Index_> > children;
};
/**
 * @endcond
 */

template<typename Index_>
struct Nclist {
/**
 * @cond
 */
    std::vector<Node<Index_> > contents;
/**
 * @endcond
 */
};

/**
 * @cond
 */
template<typename Index_, typename Position_>
Nclist<Index_> build_internal(Index_ num_ranges, Index_* subset, const Position_* start, const Position_* end) {
    std::sort(subset, subset + num_ranges, [&](Index_ l, Index_ r) -> bool {
        if (start[l] == start[r]) {
            return end[l] < end[r];
        } else {
            return start[l] < start[r];
        }
    });

    struct StackElement {
        StackElement() = default;
        StackElement(Index_ offset, Position_ end) : offset(offset), end(end) {}
        Index_ offset;
        Position_ end;
    };
    std::vector<StackElement> stack;
    Nclist<Index_> output;
    output.contents.resize(static_cast<std::size_t>(num_ranges) + 1);

    for (Index_ r = 0; r < num_ranges; ++r) {
        auto curid = subset[r];
        auto curend = end[curid];
        while (!stack.empty() && end[stack.back().id] < curend) {
            stack.pop_back();
        }

		auto landing_offset = (stack.empty() ? static_cast<Index_>(0) : stack.back().offset);
        auto& landing_node = output.contents[landing_offset]; 
        landing_node.children.emplace_back(r + 1, curid); 
        stack.emplace_back(r + 1, curend);
    }
}
/**
 * @endcond
 */

Nclist<Index_> build(Index_ num_ranges, const Index_* subset, const Position_* start, const Position_* end) {
    std::vector<Index_> copy(subset, subset + num_ranges);
    return build_internal(num_ranges, copy.data(), start, end);
}

Nclist<Index_> build(Index_ num_ranges, const Position_* start, const Position_* end) {
    std::vector<Index_> copy(num_ranges);
    std::iota(copy.begin(), copy.end(), static_cast<Index_>(0));
    return build_internal(num_ranges, copy.data(), start, end);
}

}

#endif

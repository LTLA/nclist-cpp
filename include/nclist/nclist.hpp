#ifndef NCLIST_HPP
#define NCLIST_HPP

#include <stdexcept>
#include <vector>
#include <cstddef>

namespace nclist {

template<typename Index_, typename Position_>
struct Nclist {
/**
 * @cond
 */
    struct Node {
        Index_ id;
        Position_ start, end;
        Index_ children_start;
        Index_ children_end;
        Index_ duplicates_start;
        Index_ duplicates_end;
    };
    std::vector<Node> nodes; 
    std::vector<Index_> children;
    std::vector<Index_> duplicates;
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

    // This uses offsets instead of pointers to dynamically allocated arrays.
    // We can't easily do that in C++ while still using std::vector, because
    // the type of the node is not yet complete when it references itself.
    struct WorkingNode {
        WorkingNode(Index_ id) : id(id) {}
        Index_ id = 0;
        std::vector<Index_> children;
        std::vector<Index_> duplicates;
    };
    std::vector<WorkingNode> contents;
    contents.reserve(static_cast<std::size_t>(num_ranges) + 1);
    contents.resize(1);

    struct StackElement {
        StackElement() = default;
        StackElement(Index_ offset, Position_ end) : offset(offset), end(end) {}
        Index_ offset;
        Position_ end;
    };
    std::vector<StackElement> stack;

    Position_ last_start = 0, last_end = 0;
    for (Index_ r = 0; r < num_ranges; ++r) {
        auto curid = subset[r];
        auto curend = end[curid], curstart = start[curid];
        while (!stack.empty() && stack.back().end < curend) {
            stack.pop_back();
        }

		auto landing_offset = (stack.empty() ? static_cast<Index_>(0) : stack.back().offset);
        auto& landing_node = output.contents[landing_offset]; 

        // Special handling of duplicate ranges.
        if (r && last_start == curstart && last_end == curend) {
            landing_node.duplicates.push_back(curid);
            continue;
        }

        Index_ used = contents.size();
        contents.emplace_back(curid);
        landing_node.children.push_back(used); 
        stack.emplace_back(used, curend);
        last_start = curstart;
        last_end = curend;
    }

    // Formatting it into a contiguous structure for export.

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

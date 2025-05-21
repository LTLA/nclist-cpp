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
    // First entry of 'coords' and 'nodes' refers to the root node of the tree
    // and does not reference any interval.
    std::vector<std::pair<Position_, Position_> > coords;

    struct Node {
        Index_ id = 0;
        Index_ children_start = 0;
        Index_ children_end = 0;
        Index_ duplicates_start = 0;
        Index_ duplicates_end = 0;
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
    // the type of the node is not yet complete when it references itself;
    // and I don't want to use a unique_ptr here.
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
    Index_ num_duplicates = 0;
    for (Index_ r = 0; r < num_ranges; ++r) {
        auto curid = subset[r];
        auto curend = end[curid], curstart = start[curid];

        // Special handling of duplicate intervals.
        if (r && last_start == curstart && last_end == curend) {
            landing_node.duplicates.push_back(curid);
            ++num_duplicates;
            continue;
        }

        while (!stack.empty() && stack.back().end < curend) {
            stack.pop_back();
        }
		auto landing_offset = (stack.empty() ? static_cast<Index_>(0) : stack.back().offset);
        auto& landing_node = output.contents[landing_offset]; 

        Index_ used = contents.size();
        contents.emplace_back(curid);
        landing_node.children.push_back(used); 
        stack.emplace_back(used, curend);
        last_start = curstart;
        last_end = curend;
    }

    // Depth-first traversal of the NCList tree, to convert it into a
    // contiguous structure for export. This should improve cache locality.
    Nclist<Index_, Position_> output;
    auto tree_size = contents.size(); 
    output.coords.resize(tree_size);
    output.nodes.resize(tree_size);
    for (decltype(tree_size) i = 1; i < tree_size; ++i) {
        const auto& target = contents[i];
        output.coords[i].first = start[target.id];
        output.coords[i].second = end[target.id];
    }

    output.nodes[0].children_end = contents[0].children.size();
    output.children.reserve(num_ranges - num_duplicates);
    output.duplicates.reserve(num_duplicates);

    std::vector<std::pair<Index_, std::size_t> > history(1);
    while (1) {
        auto& current_state = history.back();
        const auto& current_node = contents[current_state.first];

        if (current_state.second == current_node.children.size()) {
            history.pop_back();
            if (history.empty()) {
                break;
            } else {
                continue;
            }
        }

        auto chosen_child = current_node.children[current_state.second];
        ++current_state.second; // increment this before we call emplace_back() on history, which might trigger an allocation.

        const auto& child_node = contents[chosen_child];
        auto& child_node_out = output.nodes[chosen_child];
        child_node_out.id = child_node.id;

        if (!child_node.duplicates.empty()) {
            child_node_out.duplicates_start = output.duplicates.size();
            output.duplicates.insert(output.duplicates.end(), child_node.duplicates.begin(), child_node.duplicates.end());
            child_node_out.duplicates_end = output.duplicates.size();
        }

        if (!child_node.children.empty()) {
            child_node_out.children_start = output.children.size();
            output.children.insert(output.children.end(), child_node.children.begin(), child_node.children.end());
            child_nod_out.children_end = output.children.size();
            history.emplace_back(chosen, 0);
        }
    }

    return output;
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

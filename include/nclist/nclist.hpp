#ifndef NCLIST_HPP
#define NCLIST_HPP

#include <stdexcept>
#include <vector>
#include <cstddef>
#include <algorithm>
#include <numeric>

namespace nclist {

template<typename Index_, typename Position_>
struct Nclist {
/**
 * @cond
 */
    // First entry of 'nodes' refers to the root node of the tree and does not
    // reference any interval; it is only included to enable convenient
    // traversal of the tree by moving through 'nodes'.
    struct Node {
        Node() = default;
        Node(Index_ id) : id(id) {}
        Index_ id = 0;
        std::size_t children_start = 0;
        std::size_t children_end = 0;
        std::size_t duplicates_start = 0;
        std::size_t duplicates_end = 0;
    };
    std::vector<Node> nodes; 

    // These are the start and end positions corresponding to Node::id. We put
    // them in a separate vector for better cache locality in binary searches.
    std::vector<Position_> starts, ends;

    // These are concatenations of all the individual 'duplicates' vectors, to
    // improve cache locality during tree traversal.
    std::vector<Index_> duplicates;
/**
 * @endcond
 */
};

/**
 * @cond
 */
template<typename Index_, typename Position_>
Nclist<Index_, Position_> build_internal(Index_ num_ranges, Index_* subset, const Position_* start, const Position_* end) {
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
        WorkingNode() = default;
        WorkingNode(Index_ id) : id(id) {}
        Index_ id = 0;
        std::vector<std::size_t> children;
        std::vector<Index_> duplicates;
    };
    std::vector<WorkingNode> working_contents;
    working_contents.reserve(static_cast<std::size_t>(num_ranges) + 1);
    working_contents.resize(1);

    struct StackElement {
        StackElement() = default;
        StackElement(Index_ offset, Position_ end) : offset(offset), end(end) {}
        std::size_t offset;
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
            working_contents[stack.back().offset].duplicates.push_back(curid);
            ++num_duplicates;
            continue;
        }

        while (!stack.empty() && stack.back().end < curend) {
            stack.pop_back();
        }
		auto landing_offset = (stack.empty() ? static_cast<Index_>(0) : stack.back().offset);
        auto& landing_node = working_contents[landing_offset]; 

        auto used = working_contents.size();
        working_contents.emplace_back(curid);
        landing_node.children.push_back(used); 
        stack.emplace_back(used, curend);
        last_start = curstart;
        last_end = curend;
    }

    // Depth-first traversal of the NCList tree, to convert it into a
    // contiguous structure for export. This should improve cache locality.
    Nclist<Index_, Position_> output;
    output.nodes.reserve(working_contents.size());
    output.starts.reserve(working_contents.size());
    output.ends.reserve(working_contents.size());
    output.duplicates.reserve(num_duplicates);

    auto deposit_children = [&](Index_ tmp_node_index) -> void {
        const auto& working_child_indices = working_contents[tmp_node_index].children;
        for (auto work_index : working_child_indices) {
            const auto& working_child_node = working_contents[work_index];
            auto child_id = working_child_node.id;
            output.starts.push_back(start[child_id]);
            output.ends.push_back(end[child_id]);

            output.nodes.emplace_back(child_id);
            auto& output_child_node = output.nodes.back(); 

            // We temporarily store the working index of the kid here, to allow
            // us to cross-reference back into 'working_contents' later.
            output_child_node.children_start = work_index;

            if (!working_child_node.duplicates.empty()) {
                output_child_node.duplicates_start = output.duplicates.size();
                output.duplicates.insert(output.duplicates.end(), working_child_node.duplicates.begin(), working_child_node.duplicates.end());
                output_child_node.duplicates_end = output.duplicates.size();
            }
        }
    };
    output.nodes.resize(1);
    output.nodes[0].children_start = 1;
    deposit_children(0);
    output.nodes[0].children_end = output.nodes.size();

    std::vector<std::pair<Index_, Index_> > history;
    history.emplace_back(0, 1);
    while (1) {
        auto& current_state = history.back();
        if (current_state.second == output.nodes[current_state.first].children_end) {
            history.pop_back();
            if (history.empty()) {
                break;
            } else {
                continue;
            }
        }

        auto current_output_index = current_state.second;
        auto working_child_index = output.nodes[current_output_index].children_start;
        output.nodes[current_output_index].children_start = output.nodes.size();
        deposit_children(working_child_index);
        output.nodes[current_output_index].children_end = output.nodes.size();

        ++current_state.second; // do this before the emplace_back(), otherwise it might get reallocated.
        if (!working_contents[working_child_index].children.empty()) {
            history.emplace_back(current_output_index, 0);
        }
    }

    return output;
}
/**
 * @endcond
 */

template<typename Index_, typename Position_>
Nclist<Index_, Position_> build(Index_ num_ranges, const Index_* subset, const Position_* start, const Position_* end) {
    std::vector<Index_> copy(subset, subset + num_ranges);
    return build_internal(num_ranges, copy.data(), start, end);
}

template<typename Index_, typename Position_>
Nclist<Index_, Position_> build(Index_ num_ranges, const Position_* start, const Position_* end) {
    std::vector<Index_> copy(num_ranges);
    std::iota(copy.begin(), copy.end(), static_cast<Index_>(0));
    return build_internal(num_ranges, copy.data(), start, end);
}

}

#endif

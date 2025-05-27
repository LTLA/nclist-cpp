#ifndef NCLIST_BUILD_HPP
#define NCLIST_BUILD_HPP

#include <stdexcept>
#include <vector>
#include <cstddef>
#include <algorithm>
#include <numeric>
#include <limits>

/**
 * @file build.hpp
 * @brief Build a nested containment list.
 */

namespace nclist {

/**
 * @brief Pre-built nested containment list.
 *
 * @tparam Index_ Integer type of the subject range index.
 * @tparam Position_ Numeric type for the start/end positions of each range.
 *
 * Instances of an `Nclist` are usually created by `build()`.
 */
template<typename Index_, typename Position_>
struct Nclist {
/**
 * @cond
 */
    Index_ root_children = 0;

    // Length of 'nodes' and 'duplicates' must be less than the number of
    // ranges, so we can use Index_ as the indexing type.
    struct Node {
        Node() = default;
        Node(Index_ id) : id(id) {}
        Index_ id = 0;
        Index_ children_start = 0;
        Index_ children_end = 0;
        Index_ duplicates_start = 0;
        Index_ duplicates_end = 0;
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
Nclist<Index_, Position_> build_internal(Index_ num_subset, Index_* subset, const Position_* starts, const Position_* ends) {
    // We want to sort by increasing start but DECREASING end, so that the
    // children sort after their parents. 
    std::sort(subset, subset + num_subset, [&](Index_ l, Index_ r) -> bool {
        if (starts[l] == starts[r]) {
            return ends[l] > ends[r];
        } else {
            return starts[l] < starts[r];
        }
    });

    // This uses offsets instead of pointers to dynamically allocated arrays.
    // We can't easily do that in C++ while still using std::vector, because
    // the type of the node is not yet complete when it references itself;
    // and I don't want to use a unique_ptr here.
    // 
    // Also note that all offsets can be represented as Index_ as they point
    // into 'working_contents', which can be no longer than 'num_subset'.
    struct WorkingNode {
        WorkingNode() = default;
        WorkingNode(Index_ id) : id(id) {}
        Index_ id = std::numeric_limits<Index_>::max(); // to avoid confusion when debugging.
        std::vector<Index_> children;
        std::vector<Index_> duplicates;
    };
    WorkingNode top_level;
    std::vector<WorkingNode> working_contents;
    working_contents.reserve(num_subset);

    struct StackElement {
        StackElement() = default;
        StackElement(Index_ offset, Position_ end) : offset(offset), end(end) {}
        Index_ offset;
        Position_ end;
    };
    std::vector<StackElement> stack;

    Position_ last_start = 0, last_end = 0;
    Index_ num_duplicates = 0;
    for (Index_ r = 0; r < num_subset; ++r) {
        auto curid = subset[r];
        auto curend = ends[curid], curstart = starts[curid];

        // Special handling of duplicate intervals.
        if (r && last_start == curstart && last_end == curend) {
            working_contents[stack.back().offset].duplicates.push_back(curid);
            ++num_duplicates;
            continue;
        }

        while (!stack.empty() && stack.back().end < curend) {
            stack.pop_back();
        }
        auto& landing_node = (stack.empty() ? top_level : working_contents[stack.back().offset]); 

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

    auto deposit_children = [&](const WorkingNode& working_node) -> void {
        const auto& working_child_indices = working_node.children;
        for (auto work_index : working_child_indices) {
            const auto& working_child_node = working_contents[work_index];
            auto child_id = working_child_node.id;
            output.starts.push_back(starts[child_id]);
            output.ends.push_back(ends[child_id]);

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
    deposit_children(top_level);
    output.root_children = output.nodes.size();

    Index_ root_progress = 0;
    std::vector<std::pair<Index_, Index_> > history;
    while (1) {
        Index_ current_output_index;
        if (history.empty()) {
            if (root_progress == output.root_children) {
                break;
            }
            current_output_index = root_progress;
            ++root_progress;
        } else {
            auto& current_state = history.back();
            if (current_state.second == output.nodes[current_state.first].children_end) {
                history.pop_back();
                continue;
            }
            current_output_index = current_state.second;
            ++(current_state.second); // do this before the emplace_back(), otherwise the history might get reallocated and the reference would be dangling.
        }

        auto working_child_index = output.nodes[current_output_index].children_start; // fetching it from the temporary store location, see above.
        const auto& working_node = working_contents[working_child_index];
        Index_ first_child = output.nodes.size();
        output.nodes[current_output_index].children_start = first_child;
        deposit_children(working_node);
        output.nodes[current_output_index].children_end = output.nodes.size();

        if (!working_node.children.empty()) {
            history.emplace_back(current_output_index, first_child);
        }
    }

    return output;
}
/**
 * @endcond
 */

/**
 * @tparam Index_ Integer type of the subject range index.
 * @tparam Position_ Numeric type for the start/end positions of each range.
 * @param num_subset Number of subject ranges in the subset to include in the `Nclist`.
 * @param[in] subset Pointer to an array of length equal to `num_subset`, containing the subset of subject ranges to include in the NCList.
 * @param[in] starts Pointer to an array containing the start positions of all subject ranges.
 * This should be long enough to be addressable by any elements in `[subset, subset + num_subset)`.
 * @param[in] ends Pointer to an array containing the end positions of all subject ranges.
 * This should have the same length as the array pointed to by `starts`, where the `i`-th subject range is defined as `[starts[i], ends[i])`.
 * Note the non-inclusive nature of the end positions.
 * @return A `Nclist` containing the specified subset of subject ranges.
 */
template<typename Index_, typename Position_>
Nclist<Index_, Position_> build(Index_ num_subset, const Index_* subset, const Position_* starts, const Position_* ends) {
    std::vector<Index_> copy(subset, subset + num_subset);
    return build_internal(num_subset, copy.data(), starts, ends);
}

/**
 * @tparam Index_ Integer type of the subject range index.
 * @tparam Position_ Numeric type for the start/end position of each range.
 * @param num_ranges Number of subject ranges to include in the NCList.
 * @param[in] starts Pointer to an array of length `num_ranges`, containing the start positions of all subject ranges.
 * @param[in] ends Pointer to an array of length `num_ranges`, containing the end positions of all subject ranges.
 * The `i`-th subject range is defined as `[starts[i], ends[i])`. 
 * Note the non-inclusive nature of the end positions.
 * @return A `Nclist` containing all subject ranges.
 */
template<typename Index_, typename Position_>
Nclist<Index_, Position_> build(Index_ num_ranges, const Position_* starts, const Position_* ends) {
    std::vector<Index_> copy(num_ranges);
    std::iota(copy.begin(), copy.end(), static_cast<Index_>(0));
    return build_internal(num_ranges, copy.data(), starts, ends);
}

}

#endif

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
 * @tparam Index_ Integer type of the subject interval index.
 * @tparam Position_ Numeric type for the start/end positions of each interval.
 *
 * Instances of an `Nclist` are usually created by `build()`.
 */
template<typename Index_, typename Position_>
struct Nclist {
/**
 * @cond
 */
    // Sequence of `nodes` that are children of the root node, i.e., `nodes[i]` is a child for `i` in `[0, root_children)`.
    Index_ root_children = 0;

    struct Node {
        Node() = default;
        Node(Index_ id) : id(id) {}

        // Index of the subject interval in the user-supplied arrays. 
        Index_ id = 0;

        // Sequence of `nodes` that are children of this node, i.e., `nodes[i]` is a child for `i` in `[children_start, children_end)`.
        // Note that length of `nodes` is no greater than the number of intervals, so we can use Index_ as the indexing type.
        Index_ children_start = 0;
        Index_ children_end = 0;

        // Sequence of `duplicates` containing indices of subject intervals that are duplicates of `id`,
        // i.e., `duplicates[i]` is a duplicate interval for `i` in `[duplicates_start, duplicates_end)`.
        // Note that length of `duplicates` is no greater than the number of intervals, so we can use Index_ as the indexing type.
        Index_ duplicates_start = 0;
        Index_ duplicates_end = 0;
    };

    std::vector<Node> nodes; 

    // These are the start and end positions corresponding to Node::id,
    // i.e., `starts[i]` is equal to `subject_starts[nodes[i].id]` where `subject_starts` is the `starts` in `build()`.
    // We put them in a separate vector for better cache locality in binary searches.
    std::vector<Position_> starts, ends;

    // Concatenations of the individual `duplicates` vectors, to reduce fragmentation.
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
    // We want to sort by increasing start but DECREASING end, so that the children sort after their parents. 
    auto cmp = [&](Index_ l, Index_ r) -> bool {
        if (starts[l] == starts[r]) {
            return ends[l] > ends[r];
        } else {
            return starts[l] < starts[r];
        }
    };
    if (!std::is_sorted(subset, subset + num_subset, cmp)) {
        std::sort(subset, subset + num_subset, cmp);
    }

    struct WorkingNode {
        WorkingNode() = default;
        WorkingNode(Index_ id) : id(id) {}

        // Index of the interval, i.e., one of the elements of `subset`.
        // We set the default to the max to avoid confusion when debugging.
        Index_ id = std::numeric_limits<Index_>::max();

        // Each child is represented as an offset into `working_list`.
        // All offsets can be represented as Index_ as they point into `working_list`, which can be no longer than `num_subset`.
        std::vector<Index_> children;

        // Duplicate intervals with the same coordinates as `id`.
        std::vector<Index_> duplicates;
    };
    WorkingNode top_node;
    std::vector<WorkingNode> working_list;
    working_list.reserve(num_subset);

    struct Level {
        Level() = default;
        Level(Index_ offset, Position_ end) : offset(offset), end(end) {}
        Index_ offset; // offset into `working_list`
        Position_ end; // storing the end coordinate for better cache locality.
    };
    std::vector<Level> levels;

    Position_ last_start = 0, last_end = 0;
    Index_ num_duplicates = 0;
    for (Index_ r = 0; r < num_subset; ++r) {
        auto curid = subset[r];
        auto curend = ends[curid], curstart = starts[curid];

        // Special handling of duplicate intervals.
        if (r && last_start == curstart && last_end == curend) {
            working_list[levels.back().offset].duplicates.push_back(curid);
            ++num_duplicates;
            continue;
        }

        while (!levels.empty() && levels.back().end < curend) {
            levels.pop_back();
        }
        auto& landing_node = (levels.empty() ? top_node : working_list[levels.back().offset]); 

        auto used = working_list.size();
        working_list.emplace_back(curid);
        landing_node.children.push_back(used); 
        levels.emplace_back(used, curend);
        last_start = curstart;
        last_end = curend;
    }

    // We convert the `working_list` into the output format where the children's start/end coordinates are laid out contiguously.
    // This should make for an easier binary search and improve cache locality.
    Nclist<Index_, Position_> output;
    output.nodes.reserve(working_list.size());
    output.starts.reserve(working_list.size());
    output.ends.reserve(working_list.size());
    output.duplicates.reserve(num_duplicates);

    auto deposit_children = [&](const WorkingNode& working_node) -> void {
        const auto& working_child_indices = working_node.children;
        for (auto work_index : working_child_indices) {
            const auto& working_child_node = working_list[work_index];
            auto child_id = working_child_node.id;

            // Starts and ends are guaranteed to be sorted for all children of a given node.
            // Obviously we already sorted in order of increasing starts, and interval indices were added to each `WorkingNode::children` vector in that order.
            // For the ends, this is less obvious but any ends that are equal to or less than the previous end should be a child of that previous interval, and so would not show up here.
            output.starts.push_back(starts[child_id]);
            output.ends.push_back(ends[child_id]);

            output.nodes.emplace_back(child_id);
            auto& output_child_node = output.nodes.back(); 

            // We temporarily store the working index of the kid here, to allow us to cross-reference back into `working_list` later.
            output_child_node.children_start = work_index;

            if (!working_child_node.duplicates.empty()) {
                output_child_node.duplicates_start = output.duplicates.size();
                output.duplicates.insert(output.duplicates.end(), working_child_node.duplicates.begin(), working_child_node.duplicates.end());
                output_child_node.duplicates_end = output.duplicates.size();
            }
        }
    };
    deposit_children(top_node);
    output.root_children = output.nodes.size();

    // Performing a depth-first search to allocate the nodes in the output format.
    // This effectively does two passes for each node; once to allocate it in `Nclist::nodes`, and another pass to set its `children_start` and `children_end`.
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
        const auto& working_node = working_list[working_child_index];
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
 * @tparam Index_ Integer type of the subject interval index.
 * @tparam Position_ Numeric type for the start/end positions of each interval.
 * @param num_subset Number of subject intervals in the subset to include in the `Nclist`.
 * @param[in] subset Pointer to an array of length equal to `num_subset`, containing the subset of subject intervals to include in the NCList.
 * @param[in] starts Pointer to an array containing the start positions of all subject intervals.
 * This should be long enough to be addressable by any elements in `[subset, subset + num_subset)`.
 * @param[in] ends Pointer to an array containing the end positions of all subject intervals.
 * This should have the same length as the array pointed to by `starts`, where the `i`-th subject interval is defined as `[starts[i], ends[i])`.
 * Note the non-inclusive nature of the end positions.
 * @return A `Nclist` containing the specified subset of subject intervals.
 */
template<typename Index_, typename Position_>
Nclist<Index_, Position_> build(Index_ num_subset, const Index_* subset, const Position_* starts, const Position_* ends) {
    std::vector<Index_> copy(subset, subset + num_subset);
    return build_internal(num_subset, copy.data(), starts, ends);
}

/**
 * @tparam Index_ Integer type of the subject interval index.
 * @tparam Position_ Numeric type for the start/end position of each interval.
 * @param num_intervals Number of subject intervals to include in the NCList.
 * @param[in] starts Pointer to an array of length `num_intervals`, containing the start positions of all subject intervals.
 * @param[in] ends Pointer to an array of length `num_intervals`, containing the end positions of all subject intervals.
 * The `i`-th subject interval is defined as `[starts[i], ends[i])`. 
 * Note the non-inclusive nature of the end positions.
 * @return A `Nclist` containing all subject intervals.
 */
template<typename Index_, typename Position_>
Nclist<Index_, Position_> build(Index_ num_intervals, const Position_* starts, const Position_* ends) {
    std::vector<Index_> copy(num_intervals);
    std::iota(copy.begin(), copy.end(), static_cast<Index_>(0));
    return build_internal(num_intervals, copy.data(), starts, ends);
}

}

#endif

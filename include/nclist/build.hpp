#ifndef NCLIST_BUILD_HPP
#define NCLIST_BUILD_HPP

#include <stdexcept>
#include <vector>
#include <cstddef>
#include <algorithm>
#include <numeric>
#include <limits>
//#include <chrono>
//#include <iostream>

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
struct Triplet {
    Triplet() = default;
    Triplet(Index_ id, Position_ start, Position_ end) : id(id), start(start), end(end) {}
    Index_ id;
    Position_ start, end;
};

template<class Container_, typename Size_>
void safe_resize(Container_& container, Size_ size) {
    container.resize(size);
    if (size != container.size()) {
        throw std::runtime_error("failed to resize container to specified size");
    }
}

template<typename Index_, typename Position_>
Nclist<Index_, Position_> build_internal(std::vector<Triplet<Index_, Position_> > intervals) {
    // We want to sort by increasing start but DECREASING end, so that the children sort after their parents. 
    auto cmp = [&](const Triplet<Index_, Position_>& l, const Triplet<Index_, Position_>& r) -> bool {
        if (l.start == r.start) {
            return l.end > r.end;
        } else {
            return l.start < r.start;
        }
    };
//    auto t1 = std::chrono::high_resolution_clock::now();
    if (!std::is_sorted(intervals.begin(), intervals.end(), cmp)) {
        std::sort(intervals.begin(), intervals.end(), cmp);
    }
//    auto t2 = std::chrono::high_resolution_clock::now();
//    std::cout << "sorting takes " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << std::endl;

//    t1 = std::chrono::high_resolution_clock::now();
    auto num_intervals = intervals.size();
    typedef typename Nclist<Index_, Position_>::Node WorkingNode;
    std::vector<WorkingNode> working_list;
    working_list.reserve(num_intervals);
    std::vector<Position_> working_start, working_end;
    working_start.reserve(num_intervals);
    working_end.reserve(num_intervals);

    // This section deserves some explanation.
    // For each node in the list, we need to track its children.
    // This is most easily done by allocating a vector per node, but that is very time-consuming when we have millions of nodes.
    //
    // Instead, we recognize that the number of child indices is no greater than the number of intervals (as each interval must only have one parent).
    // We allocate a 'working_children' vector of length equal to the number of intervals.
    // The left side of the vector contains the already-processed child indices, while the right side contains the currently-processed indices.
    // The aim is to store all children in a single memory allocation that can be easily freed.
    //
    // At each level, we add child indices to the right side of the vector, moving in torwards the center.
    // Once we move past a level (i.e., we discard it from 'history'), we shift its indices from the right to the left, as no more children will be added.
    // We will always have space to perform the left shift as we know the upper bound on the number of child indices.
    // This move also exposes the indices of that level's parent on the right of the vector, allowing for further addition of new children to that parent.
    //
    // A quirk of this approach is that, because we add on the right towards the center, the children are stored in reverse order of addition.
    // This requires some later work to undo this, to report the correct order for binary search.
    //
    // We use the same approach for duplicates.
    std::vector<Index_> working_children;
    safe_resize(working_children, num_intervals);
    Index_ children_used = 0;
    std::vector<Index_> working_duplicates;
    safe_resize(working_duplicates, num_intervals);
    Index_ duplicates_used = 0;

    struct Level {
        Level() = default;
        Level(Index_ offset, Position_ end) : offset(offset), end(end) {}
        Index_ offset; // offset into `working_list`
        Position_ end; // storing the end coordinate for better cache locality.
        Index_ children_start_temp;
        Index_ children_end_temp;
        Index_ duplicates_start_temp;
        Index_ duplicates_end_temp;
    };

    std::vector<Level> levels(1);
    {
        auto& first = levels.front();
        first.children_start_temp = num_intervals;
        first.children_end_temp = num_intervals;
        first.duplicates_start_temp = num_intervals;
        first.duplicates_end_temp = num_intervals;
    }

    auto process_level = [&](const Level& curlevel) -> void {
        auto& original_node = working_list[curlevel.offset];

        if (curlevel.children_start_temp < curlevel.children_end_temp) {
            original_node.children_start = children_used;
            Index_ len = curlevel.children_end_temp - curlevel.children_start_temp;
            if (curlevel.children_start_temp > children_used) { // protect the copy, though this condition should only be false at the very end of the traversal.
                std::copy_n(working_children.begin() + curlevel.children_start_temp, len, working_children.begin() + children_used);
            }
            children_used += len;
            original_node.children_end = children_used;
        }

        if (curlevel.duplicates_start_temp < curlevel.duplicates_end_temp) {
            original_node.duplicates_start = duplicates_used;
            Index_ len = curlevel.duplicates_end_temp - curlevel.duplicates_start_temp;
            if (curlevel.duplicates_start_temp > duplicates_used) { // protect the copy, though this conditoin should only be false at the very end of the traversal.
                std::copy_n(working_duplicates.begin() + curlevel.duplicates_start_temp, len, working_duplicates.begin() + duplicates_used);
            }
            duplicates_used += len;
            original_node.duplicates_end = duplicates_used;
        }
    };

    Position_ last_start = 0, last_end = 0;
    for (const auto& interval : intervals) {
        auto curid = interval.id;
        auto curstart = interval.start;
        auto curend = interval.end;

        // Special handling of duplicate intervals.
        if (last_start == curstart && last_end == curend) {
            if (levels.size() > 1) { // i.e., we're past the start.
                auto& parent_duplicates_pos = levels.back().duplicates_start_temp;
                --parent_duplicates_pos;
                working_duplicates[parent_duplicates_pos] = curid;
                continue;
            }
        }

        while (levels.size() > 1 && levels.back().end < curend) {
            const auto& curlevel = levels.back();
            process_level(curlevel);
            levels.pop_back();
        }

        auto used = working_list.size();
        working_list.emplace_back(curid);
        working_start.push_back(curstart);
        working_end.push_back(curend);

        auto parent_children_pos = levels.back().children_start_temp;
        auto parent_duplicates_pos = levels.back().duplicates_start_temp;
        --parent_children_pos;
        working_children[parent_children_pos] = used;
        levels.back().children_start_temp = parent_children_pos;

        levels.emplace_back(used, curend);
        auto& latest = levels.back();
        latest.children_start_temp = parent_children_pos;
        latest.children_end_temp = parent_children_pos;
        latest.duplicates_start_temp = parent_duplicates_pos;
        latest.duplicates_end_temp = parent_duplicates_pos;

        last_start = curstart;
        last_end = curend;
    }

    while (levels.size() > 1) { // processing all remaining levels except for the root node, which we'll handle separately.
        const auto& curlevel = levels.back();
        process_level(curlevel);
        levels.pop_back();
    }

    intervals.clear(); // freeing up some memory for more allocations in the next section.
    intervals.shrink_to_fit();

//    t2 = std::chrono::high_resolution_clock::now();
//    std::cout << "first pass takes " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << std::endl;

//    t1 = std::chrono::high_resolution_clock::now();

    // We convert the `working_list` into the output format where the children's start/end coordinates are laid out contiguously.
    // This should make for an easier binary search (allowing us to use std::lower_bound and friends) and improve cache locality.
    // We do this by traversing the list in a depth-first manner and adding all children of each node to the output vectors.
    Nclist<Index_, Position_> output;
    output.nodes.reserve(working_list.size());
    output.starts.reserve(working_list.size());
    output.ends.reserve(working_list.size());
    output.duplicates.reserve(duplicates_used);

    auto process_children = [&](Index_ from, Index_ to) -> void {
        // Remember that we inserted children and duplicates in reverse order into their working vectors.
        // So when we copy, we do so in reverse order to cancel out the reversal.
        for (Index_ i = to; i > from; --i) {
            auto child = working_children[i - 1];
            output.nodes.push_back(working_list[child]);

            // Starts and ends are guaranteed to be sorted for all children of a given node (after we cancel out the reversal).
            // Obviously we already sorted in order of increasing starts, and interval indices were added to each node's children in that order.
            // For the ends, this is less obvious but any end that is equal to or less than the previous end should be a child of that previous interval and should not show up here.
            output.starts.push_back(working_start[child]);
            output.ends.push_back(working_end[child]);

            auto& current = output.nodes.back();
            if (current.duplicates_start != current.duplicates_end) {
                auto old_start = current.duplicates_start, old_end = current.duplicates_end;
                current.duplicates_start = output.duplicates.size();
                output.duplicates.insert(
                    output.duplicates.end(),
                    working_duplicates.rbegin() + (num_intervals - old_end),
                    working_duplicates.rbegin() + (num_intervals - old_start)
                );
                current.duplicates_end = output.duplicates.size();
            }
        }
    };

    // Processing the root node here, to avoid an unnecessary extra shift within working_children.
    // Note that root node doesn't have any duplicates so we only need to consider its children here.
    process_children(levels.front().children_start_temp, levels.front().children_end_temp);
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
            if (current_state.first == current_state.second) {
                history.pop_back();
                continue;
            }
            current_output_index = current_state.first;
            ++(current_state.first); // do this before the emplace_back(), otherwise the history might get reallocated and the reference would be dangling.
        }

        auto old_start = output.nodes[current_output_index].children_start;
        auto old_end = output.nodes[current_output_index].children_end;
        if (old_start != old_end) {
            Index_ first_child = output.nodes.size();
            output.nodes[current_output_index].children_start = first_child;
            process_children(old_start, old_end);
            Index_ past_last_child = output.nodes.size();
            output.nodes[current_output_index].children_end = past_last_child;
            history.emplace_back(first_child, past_last_child);
        }
    }
//    t2 = std::chrono::high_resolution_clock::now();
//    std::cout << "second pass takes " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << std::endl;

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
    std::vector<Triplet<Index_, Position_> > intervals;
    intervals.reserve(num_subset);
    for (Index_ s = 0; s < num_subset; ++s) {
        intervals.emplace_back(subset[s], starts[s], ends[s]);
    }
    return build_internal(std::move(intervals));
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
//    auto t1 = std::chrono::high_resolution_clock::now();
    std::vector<Triplet<Index_, Position_> > intervals;
    intervals.reserve(num_intervals);
    for (Index_ i = 0; i < num_intervals; ++i) {
        intervals.emplace_back(i, starts[i], ends[i]);
    }
//    auto t2 = std::chrono::high_resolution_clock::now();
//    std::cout << "creating the thing takes " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << std::endl;
    return build_internal(std::move(intervals));
}

}

#endif

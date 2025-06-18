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
    std::vector<Index_> working_children;
    working_children.reserve(num_intervals); // splurge a bit of memory here and for duplicates to avoid reallocations.
    std::vector<Index_> working_duplicates;
    working_duplicates.reserve(num_intervals);

    struct Level {
        Level() = default;
        Level(Index_ offset, Position_ end) : offset(offset), end(end) {}
        Index_ offset; // offset into `working_list`
        Position_ end; // storing the end coordinate for better cache locality.
        std::vector<Index_> children, duplicates;
    };
    std::vector<Level> levels(1);
    decltype(levels.size()) in_use = 0;

    Position_ last_start = 0, last_end = 0;
    for (decltype(num_intervals) i = 0; i < num_intervals; ++i) {
        const auto& interval = intervals[i];
        auto curid = interval.id;
        auto curstart = interval.start;
        auto curend = interval.end;

        // Special handling of duplicate intervals.
        if (last_start == curstart && last_end == curend) {
            if (i > 0) {
                levels[in_use].duplicates.push_back(curid); // in_use must be >0 at this point.
                continue;
            }
        }

        while (in_use > 0 && levels[in_use].end == curend) {
            auto& curlevel = levels[in_use];
            auto& original_node = working_list[curlevel.offset];
            
            // Once we no longer need a level, we can't add to its children or duplicates;
            // so we can safely shift these to the global 'working_children' and 'working_duplicates'.
            // This allows us to reuse the children and duplicates vectors for the next level.
            if (!curlevel.children.empty()) {
                original_node.children_start = working_children.size();
                working_children.insert(working_children.end(), curlevel.children.begin(), curlevel.children.end());
                original_node.children_end = working_children.size();
                curlevel.children.clear();
            }

            if (!curlevel.duplicates.empty()) {
                original_node.duplicates_start = working_duplicates.size();
                working_duplicates.insert(working_duplicates.end(), curlevel.duplicates.begin(), curlevel.duplicates.end());
                original_node.duplicates_end = working_duplicates.size();
                curlevel.duplicates.clear();
            }

            // Don't pop_back() so that we can reuse the already-allocated memory for children and duplicates.
            --in_use;
        }

        auto used = working_list.size();
        working_list.emplace_back(curid);
        working_start.push_back(curstart);
        working_end.push_back(curend);

        levels[in_use].children.push_back(used);
        ++in_use;
        if (in_use == levels.size()) {
            levels.emplace_back(used, curend);
        } else {
            levels[in_use].offset = used;
            levels[in_use].end = curend;
        }

        last_start = curstart;
        last_end = curend;
    }

    intervals.clear();
    intervals.shrink_to_fit();

//    t2 = std::chrono::high_resolution_clock::now();
//    std::cout << "first pass takes " << std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count() << std::endl;

//    t1 = std::chrono::high_resolution_clock::now();

    // We convert the `working_list` into the output format where the children's start/end coordinates are laid out contiguously.
    // This should make for an easier binary search and improve cache locality.
    Nclist<Index_, Position_> output;
    output.nodes.reserve(working_list.size());
    output.starts.reserve(working_list.size());
    output.ends.reserve(working_list.size());
    output.duplicates.reserve(working_duplicates.size());

    // Processing the root node. The node itself doesn't have any duplicates so we only consider the 'children' field.
    for (auto child : levels.front().children) {
        output.nodes.push_back(working_list[child]);
        output.starts.push_back(working_start[child]);
        output.ends.push_back(working_end[child]);

        auto& current = output.nodes.back();
        if (current.duplicates_start != current.duplicates_end) {
            auto old_start = current.duplicates_start, old_end = current.duplicates_end;
            current.duplicates_start = output.duplicates.size();
            output.duplicates.insert(output.duplicates.end(), working_duplicates.begin() + old_start, working_duplicates.begin() + old_end);
            current.duplicates_end = output.duplicates.size();
        }
    }
    output.root_children = output.nodes.size();
    levels.clear();
    levels.shrink_to_fit();

    // Performing a depth-first search that does two passes for each node; once to put it in `Nclist::nodes`, and another to set its `children_start` and `children_end`.
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

            for (auto child_idx = old_start; child_idx < old_end; ++child_idx) {
                output.nodes.push_back(working_list[child_idx]);
                output.starts.push_back(working_start[child_idx]);
                output.ends.push_back(working_end[child_idx]);

                auto& current = output.nodes.back();
                if (current.duplicates_start != current.duplicates_end) {
                    auto old_start = current.duplicates_start, old_end = current.duplicates_end;
                    current.duplicates_start = output.duplicates.size();
                    output.duplicates.insert(output.duplicates.end(), working_duplicates.begin() + old_start, working_duplicates.begin() + old_end);
                    current.duplicates_end = output.duplicates.size();
                }
            }

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

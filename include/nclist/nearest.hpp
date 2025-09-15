#ifndef NCLIST_NEAREST_HPP
#define NCLIST_NEAREST_HPP

#include <vector>
#include <algorithm>
#include <optional>
#include <limits>

#include "build.hpp"
#include "utils.hpp"

/**
 * @file nearest.hpp
 * @brief Find the nearest interval.
 */

namespace nclist {

/**
 * @brief Workspace for `nearest()`.
 *
 * @tparam Index_ Integer type of the subject interval index.
 *
 * This holds intermediate data structures that can be re-used across multiple calls to `nearest()` to avoid reallocations.
 */
template<typename Index_>
struct NearestWorkspace {
    /**
     * @cond
     */
    struct State {
        State() = default;
        State(Index_ cat, Index_ cend, bool skip) : child_at(cat), child_end(cend), skip_search(skip) {}
        Index_ child_at = 0, child_end = 0;
        bool skip_search = false;
    };
    std::vector<State> history;
    /**
     * @endcond
     */
};

/**
 * @brief Parameters for `nearest()`.
 * @tparam Position_ Numeric type of the start/end positions of each interval.
 */
template<typename Position_>
struct NearestParameters {
    /**
     * Whether to quit immediately upon identifying the nearest subject interval to the query interval.
     * In such cases, `matches` will contain one arbitrarily chosen subject interval that is nearest to the query.
     */
    bool quit_on_first = false;

    /**
     * Whether to consider immediately-adjacent subject intervals to be equally "nearest" to the query as an overlapping subject interval.
     * If `true`, both overlapping and immediately-adjacent subject intervals (i.e., a gap of zero) will be reported in `matches`.
     * Otherwise, immediately-adjacent subjects will only be reported if overlapping subjects are not present.
     */
    bool adjacent_equals_overlap = false;
};

/**
 * @cond
 */
template<typename Index_, typename Position_>
void nearest_before(
    const Nclist<Index_, Position_>& subject,
    const Index_ root_index,
    const Position_ end_position,
    const bool quit_on_first,
    std::vector<Index_>& matches)
{
    Index_ current_subject = root_index;
    while (1) {
        const auto& current_node = subject.nodes[current_subject];
        matches.push_back(current_node.id);
        if (quit_on_first) {
            return;
        }
        if (current_node.duplicates_start != current_node.duplicates_end) {
            matches.insert(matches.end(), subject.duplicates.begin() + current_node.duplicates_start, subject.duplicates.begin() + current_node.duplicates_end);
        }
        if (current_node.children_start == current_node.children_end) {
            return;
        }
        current_subject = current_node.children_end - 1;
        if (subject.ends[current_subject] != end_position) {
            return;
        }
    }
}

template<typename Index_, typename Position_>
void nearest_after(
    const Nclist<Index_, Position_>& subject,
    const Index_ root_index,
    const Position_ start_position,
    const bool quit_on_first,
    std::vector<Index_>& matches)
{
    Index_ current_subject = root_index;
    while (1) {
        const auto& current_node = subject.nodes[current_subject];
        matches.push_back(current_node.id);
        if (quit_on_first) {
            return;
        }
        if (current_node.duplicates_start != current_node.duplicates_end) {
            matches.insert(matches.end(), subject.duplicates.begin() + current_node.duplicates_start, subject.duplicates.begin() + current_node.duplicates_end);
        }
        if (current_node.children_start == current_node.children_end) {
            return;
        }
        current_subject = current_node.children_start;
        if (subject.starts[current_subject] != start_position) {
            return;
        }
    }
}

template<typename Index_, typename Position_>
Index_ nearest_overlaps(
    const Nclist<Index_, Position_>& subject,
    const Position_ query_start,
    const Position_ query_end,
    const bool quit_on_first,
    const bool adjacent_equals_overlap,
    NearestWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    /****************************************
     * # Default 
     *
     * This section is the same as that of overlaps_any().
     * Our aim is to find overlaps to a subject interval `i` where `subject_starts[i] < query_end` and `query_start < subject_ends[i]`.
     *
     * At each node of the NCList, we search for the first child where the `subject_ends` is greater than `query_start` (as ends are non-inclusive).
     * Earlier "sibling" intervals cannot overlap with the query interval, nor can their children.
     * We do so using a binary search (std::upper_bound) on `subject_ends` - recall that these are sorted for children of each node.
     *
     * We then iterate until the first interval `j` where `query_end <= subject_starts[j]`, at which point we stop.
     * This is because `j`, the children of `j`, nor any sibling intervals after `j` can overlap with the query interval, so there's no point in traversing those nodes. 
     * Any subject interval encountered during this iteration is reported as an overlap, and we process its children in the same manner.
     *
     * For a modest efficiency boost, we consider the case where `query_start < subject_starts[i]` for node `i`.
     * In such cases, `subject_ends` for all children of `i` must also satisfy `query_start < subject_ends[i]`, in which case the binary search can be skipped.
     * Moreover, all descendent intervals of `i` must end after `query_start`, so the binary search can be skipped for the entire lineage of the NCList.
     *
     * # Adjacent == overlaps
     *
     * For historical reasons, we can also report intervals that are immediately adjacent to the query.
     *
     * At each node of the NCList, we check whether there is a child where `subject_ends == query_start`, i.e., the subject immediately precedes the query.
     * This is quite easily accomplished as we already have the first child where `subject_ends > query_start`, so we just backtrack by 1 if available.
     * We then recurse within the preceding-adjacent child to see if there are any descendents that are also immediately adjacent, see `nearest_before()`.
     *
     * Similarly, to find subjects that are immediately following the query, we check whether there is a child where `query_end == subject_starts`.
     * We already do this check as part of the termination of the iterations, so it's not much extra work involved.
     * We then recurse within the following-adjacent child to see if there are any descendents that are also immediately adjacent, see `nearest_after()`.
     *
     * If `query_start < subject_starts[i]` for node `i`, we can skip the check for preceding subjects.
     * There are no subjects before the query at that level of the tree.
     */

    const auto find_first_child = [&](const Index_ children_start, const Index_ children_end) -> Index_ {
        const auto ebegin = subject.ends.begin();
        const auto estart = ebegin + children_start; 
        const auto eend = ebegin + children_end;
        return std::upper_bound(estart, eend, query_start) - ebegin;
    };

    const auto can_skip_search = [&](const Position_ subject_start) -> bool {
        return subject_start > query_start;
    };

    const auto is_finished = [&](const Position_ subject_start) -> bool {
        return subject_start >= query_end;
    };

    Index_ root_child_at = 0;
    const bool root_skip_search = can_skip_search(subject.starts[0]);
    if (!root_skip_search) {
        root_child_at = find_first_child(0, subject.root_children);
        if (adjacent_equals_overlap && root_child_at) {
            const Index_ previous_child = root_child_at - 1;
            if (query_start == subject.ends[previous_child]) { 
                nearest_before(subject, previous_child, query_start, quit_on_first, matches);
                if (quit_on_first && !matches.empty()) {
                    return root_child_at;
                }
            }
        }
    }

    workspace.history.clear();
    while (1) {
        Index_ current_subject;
        bool skip_search;

        if (workspace.history.empty()) {
            if (root_child_at == subject.root_children) {
                break;
            } else {
                const Position_ next_start = subject.starts[root_child_at];
                if (is_finished(next_start)) {
                    if (adjacent_equals_overlap && next_start == query_end) {
                        nearest_after(subject, root_child_at, query_end, quit_on_first, matches);
                        // No need to return early if `quit_on_first = true`, we're already breaking out of the loop anyway.
                    }
                    break;
                }
            }
            current_subject = root_child_at;
            skip_search = root_skip_search;
            ++root_child_at;

        } else {
            auto& current_state = workspace.history.back();
            if (current_state.child_at == current_state.child_end) {
                workspace.history.pop_back();
                continue;
            } else {
                const Position_ next_start = subject.starts[current_state.child_at];
                if (is_finished(next_start)) {
                    if (adjacent_equals_overlap && next_start == query_end) {
                        nearest_after(subject, current_state.child_at, query_end, false, matches);
                        // `quit_on_first` must be false because, if it were true, we would have returned in a previous iteration;
                        // we can't get to this point in the function without overlapping a parent interval.
                    }
                    workspace.history.pop_back();
                    continue;
                }
            }
            current_subject = current_state.child_at;
            skip_search = current_state.skip_search;
            ++(current_state.child_at); // do this before the emplace_back(), otherwise the history might get reallocated and the reference would be dangling.
        }

        const auto& current_node = subject.nodes[current_subject];

        matches.push_back(current_node.id);
        if (quit_on_first) {
            break;
        }
        if (current_node.duplicates_start != current_node.duplicates_end) {
            matches.insert(matches.end(), subject.duplicates.begin() + current_node.duplicates_start, subject.duplicates.begin() + current_node.duplicates_end);
        }

        if (current_node.children_start != current_node.children_end) {
            if (skip_search) {
                workspace.history.emplace_back(current_node.children_start, current_node.children_end, true);
            } else {
                const Index_ start_pos = find_first_child(current_node.children_start, current_node.children_end);
                if (adjacent_equals_overlap && start_pos > current_node.children_start) {
                    const Index_ previous_child = start_pos - 1;
                    if (query_start == subject.ends[previous_child]) { 
                        nearest_before(subject, previous_child, query_start, false, matches);
                        // `quit_on_first` must be false because, if it were true, we would have returned already;
                        // we can't get to this point in the function without overlapping a parent interval.
                    }
                }
                if (start_pos != current_node.children_end) {
                    workspace.history.emplace_back(start_pos, current_node.children_end, can_skip_search(subject.starts[start_pos]));
                }
            }
        }
    }

    return root_child_at;
}
/**
 * @endcond
 */

/**
 * Find subject intervals that are nearest to the query interval. 
 * If any overlaps are present, all overlapping intervals are reported.
 * If no overlaps are present, the subject interval with the smallest gap to the query is reported.
 * A gap is defined as the distance between the query start and subject end (for queries after the subject) or the subject start and the query end (otherwise). 
 * If multiple subjects have the same gap, all ties are reported.
 *
 * This function is based on its counterpart of the same name in the [**IRanges** R/Bioconductor package](https://bioconductor.org/packages/IRanges).
 * Users should set `NearestParameters::adjacent_equals_overlap = true` to obtain the same results as the R package, 
 *
 * @tparam Index_ Integer type of the subject interval index.
 * @tparam Position_ Numeric type for the start/end positions of each interval.
 *
 * @param subject An `Nclist` of subject intervals, typically built with `build()`. 
 * @param query_start Start of the query interval.
 * @param query_end Non-inclusive end of the query interval.
 * @param params Parameters for the search.
 * @param workspace Workspace for intermediate data structures.
 * This can be default-constructed and re-used across `nearest()` calls.
 * @param[out] matches On output, vector of indices of the nearest subject intervals to the query interval.
 * Indices are reported in arbitrary order.
 */
template<typename Index_, typename Position_>
void nearest(
    const Nclist<Index_, Position_>& subject,
    const Position_ query_start,
    const Position_ query_end,
    const NearestParameters<Position_>& params,
    NearestWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    matches.clear();
    if (subject.root_children == 0) {
        return;
    }

    const auto root_index = nearest_overlaps(
        subject,
        query_start,
        query_end,
        params.quit_on_first,
        params.adjacent_equals_overlap,
        workspace,
        matches
    );
    if (!matches.empty()) {
        return;
    }

    /****************************************
     * If there are no overlaps, checking the distance to the first subject interval before or after the query.
     *
     * `root_index` is initially defined in `nearest_overlaps()` as the upper bound of `query_start` on the subject ends of the root node of the NCList.
     * If there are no overlaps, this definition won`t change and the same value will be returned to `nearest()`.
     * At this point, `root_index` defines the first subject interval on the root node that starts after the query.
     * Conversely, `root_index - 1` is the last subject interval on the root node that ends before the query.
     *
     * Thus, to find the nearest following interval, only the `root_index`-th interval on the root node needs to be considered as it will have the start closest to `query_end`.
     * All later intervals on the root node will have later starts, otherwise they would be nested within the `root_index`-th interval.
     * We still need to check the descendents of the `root_index`-th interval but we only need to care about the first child at each level of the lineage.
     * In particular, we check whether it has the same start coordinate as its parent;
     * if not, we know that the rest of that lineage will start after the `root_index`-th interval and we terminate the search.
     *
     * Similarly, to find the nearest preceding interval, only the `root_index - 1`-th interval on the root node needs to be considered as it will have the end closest to `query_start`.
     * All earlier intervals on the root node will have earlier ends, otherwise they would be nested within the `root_index - 1`-th interval.
     * We still need to check the descendents of the `root_index - 1`-th interval but we only need to care about the last child at each level of the lineage.
     * In particular, we check whether it has the same end coordinate as its parent; 
     * if not, we know that the rest of that lineage will end before the `root_index - 1`-th interval and we terminate the search.
     *
     * Note that this function behaves a little strangely when a zero-width query is supplied and there is an identical zero-width subject interval. 
     * Specifically, the latter may be treated as preceding or following the former, depending on whether they are nested in other subject intervals.
     * Fortunately, this doesn`t have any bearing on the final outcome as it will still be the nearest interval to the query in either case.
     *
     * If `adjacent_equals_overlap = true`, the same logic applies as we still don't have any overlaps if we get to this part of the function.
     * The only difference is that our gaps are now guaranteed to be non-zero if no adjacent/overlapping intervals were found in `nearest_overlaps()`.
     *
     ****************************************/

    std::optional<Position_> to_previous, to_next; 
    if (root_index) {
        to_previous = query_start - subject.ends[root_index - 1];
    }
    if (root_index < subject.root_children) {
        to_next = subject.starts[root_index] - query_end; 
    }

    if (to_previous.has_value() && (!to_next.has_value() || *to_previous <= *to_next)) {
        const Index_ previous_child = root_index - 1;
        nearest_before(subject, previous_child, subject.ends[previous_child], params.quit_on_first, matches);
        if (!matches.empty() && params.quit_on_first) {
            return;
        }
    }
    if (to_next.has_value() && (!to_previous.has_value() || *to_next <= *to_previous)) {
        nearest_after(subject, root_index, subject.starts[root_index], params.quit_on_first, matches);
        if (!matches.empty() && params.quit_on_first) {
            return;
        }
    }
}

}

#endif

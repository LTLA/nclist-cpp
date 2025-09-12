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
};

/**
 * @cond
 */
// See the logic in overlaps_any() for details.
template<typename Index_, typename Position_>
Index_ nearest_overlaps(
    const Nclist<Index_, Position_>& subject,
    const Position_ query_start,
    const Position_ query_end,
    const NearestParameters<Position_>& params,
    NearestWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
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
    }

    workspace.history.clear();
    while (1) {
        Index_ current_subject;
        bool skip_search;
        if (workspace.history.empty()) {
            if (root_child_at == subject.root_children || is_finished(subject.starts[root_child_at])) {
                break;
            }
            current_subject = root_child_at;
            skip_search = root_skip_search;
            ++root_child_at;
        } else {
            auto& current_state = workspace.history.back();
            if (current_state.child_at == current_state.child_end || is_finished(subject.starts[current_state.child_at])) {
                workspace.history.pop_back();
                continue;
            }
            current_subject = current_state.child_at;
            skip_search = current_state.skip_search;
            ++(current_state.child_at); // do this before the emplace_back(), otherwise the history might get reallocated and the reference would be dangling.
        }

        const auto& current_node = subject.nodes[current_subject];

        matches.push_back(current_node.id);
        if (params.quit_on_first) {
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
                if (start_pos != current_node.children_end) {
                    workspace.history.emplace_back(start_pos, current_node.children_end, can_skip_search(subject.starts[start_pos]));
                }
            }
        }
    }

    return root_child_at;
}

template<typename Index_, typename Position_>
void nearest_before(
    const Nclist<Index_, Position_>& subject,
    const Index_ root_position,
    const NearestParameters<Position_>& params,
    std::vector<Index_>& matches)
{
    Index_ current_subject = root_position;
    const Position_ closest = subject.ends[current_subject];
    while (1) {
        const auto& current_node = subject.nodes[current_subject];
        matches.push_back(current_node.id);
        if (params.quit_on_first) {
            return;
        }
        if (current_node.duplicates_start != current_node.duplicates_end) {
            matches.insert(matches.end(), subject.duplicates.begin() + current_node.duplicates_start, subject.duplicates.begin() + current_node.duplicates_end);
        }
        if (current_node.children_start == current_node.children_end) {
            return;
        }
        current_subject = current_node.children_end - 1;
        if (subject.ends[current_subject] != closest) {
            return;
        }
    }
}

template<typename Index_, typename Position_>
void nearest_after(
    const Nclist<Index_, Position_>& subject,
    const Index_ root_position,
    const NearestParameters<Position_>& params,
    std::vector<Index_>& matches)
{
    Index_ current_subject = root_position;
    const Position_ closest = subject.starts[current_subject];
    while (1) {
        const auto& current_node = subject.nodes[current_subject];
        matches.push_back(current_node.id);
        if (params.quit_on_first) {
            return;
        }
        if (current_node.duplicates_start != current_node.duplicates_end) {
            matches.insert(matches.end(), subject.duplicates.begin() + current_node.duplicates_start, subject.duplicates.begin() + current_node.duplicates_end);
        }
        if (current_node.children_start == current_node.children_end) {
            return;
        }
        current_subject = current_node.children_start;
        if (subject.starts[current_subject] != closest) {
            return;
        }
    }
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
 * (This logic is inspired by the behavior of the function of the same name in the [**IRanges** R/Bioconductor package](https://bioconductor.org/packages/IRanges).)
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

    const auto root_index = nearest_overlaps(subject, query_start, query_end, params, workspace, matches);
    if (!matches.empty()) {
        return;
    }

    /****************************************
     * If there are no overlaps, checking the distance to the first subject interval before or after the query.
     *
     * 'root_index' is initially defined in nearest_overlaps() as the upper bound of 'query_start' on the subject ends of the root node of the NCList.
     * If there are no overlaps, this definition won't change and the same value will be returned to nearest().
     * At this point, 'root_index' defines the first subject interval on the root node that starts after the query.
     * Conversely, 'root_index - 1' is the last subject interval on the root node that ends before the query.
     *
     * Thus, to find the nearest following interval, only the 'root_index'-th interval on the root node needs to be considered as it will have the start closest to 'query_end'.
     * All later intervals on the root node will have later starts, otherwise they would be nested within the 'root_index`-th interval.
     * We still need to check the descendents of the 'root_index'-th interval but we only need to care about the first child at each level of the lineage.
     * In particular, we check whether it has the same start coordinate as its parent;
     * if not, we know that the rest of that lineage will start after the 'root_index'-th interval and we terminate the search.
     *
     * Similarly, to find the nearest preceding interval, only the 'root_index - 1'-th interval on the root node needs to be considered as it will have the end closest to 'query_start'.
     * All earlier intervals on the root node will have earlier ends, otherwise they would be nested within the 'root_index - 1`-th interval.
     * We still need to check the descendents of the 'root_index - 1'-th interval but we only need to care about the last child at each level of the lineage.
     * In particular, we check whether it has the same end coordinate as its parent; 
     * if not, we know that the rest of that lineage will end before the 'root_index - 1'-th interval and we terminate the search.
     *
     * Note that this function behaves a little strangely when a zero-width query is supplied and there is an identical zero-width subject interval. 
     * Specifically, the latter may be treated as preceding or following the former, depending on whether they are nested in other subject intervals.
     * Fortunately, this doesn't have any bearing on the final outcome as it will still the nearest interval to the query in either case.
     *
     ****************************************/

    std::optional<Position_> to_previous, to_next; 
    if (root_index) {
        to_previous = query_start - subject.ends[root_index - 1];
    }
    if (root_index < subject.root_children) {
        to_next = subject.starts[root_index] - query_end; 
    }

    if (!to_next.has_value() || (to_previous.has_value() && *to_previous <= *to_next)) {
        nearest_before(subject, root_index - 1, params, matches);
        if (!matches.empty() && params.quit_on_first) {
            return;
        }
    }
    if (!to_previous.has_value() || (to_next.has_value() && *to_next <= *to_previous)) {
        nearest_after(subject, root_index, params, matches);
        if (!matches.empty() && params.quit_on_first) {
            return;
        }
    }
}

}

#endif

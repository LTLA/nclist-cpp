#ifndef NCLIST_OVERLAPS_EQUAL_HPP
#define NCLIST_OVERLAPS_EQUAL_HPP

#include <vector>
#include <algorithm>

#include "build.hpp"
#include "utils.hpp"

/**
 * @file overlaps_equal.hpp
 * @brief Find intervals with the same start and end positions.
 */

namespace nclist {

/**
 * @brief Workspace for `overlaps_equal()`.
 *
 * @tparam Index_ Integer type of the subject interval index.
 *
 * This holds intermediate data structures that can be re-used across multiple calls to `overlaps_equal()` to avoid reallocations.
 */
template<typename Index_>
struct OverlapsEqualWorkspace {
    /**
     * @cond
     */
    struct State {
        State() = default;
        State(Index_ cat, Index_ cend) : child_at(cat), child_end(cend) {}
        Index_ child_at = 0, child_end = 0;
    };
    std::vector<State> history;
    /**
     * @endcond
     */
};

/**
 * @brief Parameters for `overlaps_equal()`.
 * @tparam Position_ Numeric type for the start/end positions of each interval.
 */
template<typename Position_>
struct OverlapsEqualParameters {
    /**
     * Maximum gap between the starts/ends of query and subject intervals.
     * An overlap is reported between a query/subject pair if the gap between starts and the gap between ends are both equal to or less than `max_gap`. 
     */
    Position_ max_gap = 0;

    /**
     * Minimum overlap between query and subject intervals.
     * An overlap will not be reported if the length of the overlapping subinterval is less than `min_overlap`.
     */
    Position_ min_overlap = 0;

    /**
     * Whether to quit immediately upon identifying an overlap with the query interval.
     * In such cases, `matches` will contain one arbitrarily chosen subject interval that overlaps with the query.
     */
    bool quit_on_first = false;
};


/**
 * Find subject intervals with the same start and end positions as the query interval.
 * By default, given a subject interval `[subject_start, subject_end)`, an overlap is considered if `subject_start == query_end` and `query_start == subject_end`.
 * This behavior can be tuned with parameters in `params`.
 *
 * @tparam Index_ Integer type of the subject interval index.
 * @tparam Position_ Numeric type for the start/end positions of each interval.
 *
 * @param subject An `Nclist` of subject intervals, typically built with `build()`. 
 * @param query_start Start of the query interval.
 * @param query_end Non-inclusive end of the query interval.
 * @param params Parameters for the search.
 * @param workspace Workspace for intermediate data structures.
 * This can be default-constructed and re-used across `overlaps_equal()` calls.
 * @param[out] matches On output, vector of subject interval indices that overlap with the query interval.
 * Indices are reported in arbitrary order.
 */
template<typename Index_, typename Position_>
void overlaps_equal(
    const Nclist<Index_, Position_>& subject,
    const Position_ query_start,
    const Position_ query_end,
    const OverlapsEqualParameters<Position_>& params,
    OverlapsEqualWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    matches.clear();
    if (subject.root_children == 0) {
        return;
    }

    /****************************************
     * # Default
     *
     * Our aim is to find overlaps to a subject interval `i` where `subject_starts[i] == query_start` and `subject_ends[i] == query_end`. 
     *
     * At each node of the NCList, we search for the first child where the `subject_ends` is greater or equal to `query_end`. 
     * Earlier "sibling" intervals must have earlier end positions that cannot be equal to that of the query interval - nor can their children.
     * We do so using a binary search (std::lower_bound) on `subject_ends` - recall that these are sorted for children of each node.
     *
     * We then iterate until the first interval `j` where `query_start < subject_starts[j]`, at which point we stop.
     * None of `j`, the children of `j`, nor any sibling intervals after `j` can have an start position equal to the query interval, so there's no point in traversing those nodes. 
     * Any subject interval encountered during this iteration with equal start/end positions to the query is reported in `matches`.
     * Note that we can return early if we find a matching interval, as exactly one node will contain all duplicate intervals with the same start/end positions, so there's no point continuing.
     * Otherwise, we repeat the search on the children of all subject intervals encountered during iteration, as one of them might have equal start/end positions.
     *
     * Unlike `overlaps_any()`, there is no ability to skip the binary search for the descendents of a node. 
     * Sure, the start positions of all descendents are no less than the node's subject interval's start position,
     * but this doesn't say much about the comparison between the subject and query end positions.
     *
     * # Max gap
     *
     * Here, our aim is to find subject intervals where `abs(query_end - subject_ends[i]) <= max_gap` and `abs(query_start - subject_starts[i]) <= max_gap`.
     * This follows much the same logic as in the default case with some adjustments:
     *
     * - We consider an "effective" query end as `effective_query_end = query_end - max_gap`.
     * - We then perform a binary search to find the first `subject_ends` that is greater than or equal to `effective_query_start`.
     * - We stop iteration once `subject_starts[j] - query_start > max_gap`, after which we know that all children's starts must lie beyond `max_gap`.
     * - Any subject interval encountered during this iteration with start/end positions that satisfies `max_gap` is reported in `matches`.
     * - We do not return early if we find a matching interval, as its children or siblings can still match within `max_gap`.
     *
     * # Min overlap
     *
     * Here, we apply the extra restriction that the overlapping subinterval must have length greater than `min_overlap`.
     * This follows the same logic as the default case, with some modifications:
     *
     * - We stop iteration once `query_end - subject_starts[j] < min_overlap`, after which we know that all children cannot achieve `min_overlap`.
     * - We apply an extra check on each subject interval to determine if it achieves `min_overlap` before reporting it in `matches`.
     * - If the length of the overlapping subinterval is less than `min_overlap`, we do not report the subject interval in `matches`.
     *   Morever, we skip traversal of that node's children, as the children must be smaller and will not satisfy `min_overlap` anyway. 
     *
     * We return early if the length of the query itself is less than `min_overlap`, as no overlap will be satisfactory.
     *
     * Note that we do not use an effective query end for the binary search.
     * The comparison between query/subject ends doesn't say anything about the length of the overlap subinterval.
     *
     * These modifications are mostly orthogonal to those required when `max_gap > 0`, so can be combined without much effort. 
     * We stop iterations when either of the stopping criteria for `max_gap` or `min_overlap` are satisfied.
     *
     ****************************************/

    if (params.min_overlap > 0) {
        if (query_end - query_start < params.min_overlap) {
            return;
        }
    }

    Position_ effective_query_end = query_end;
    if (params.max_gap > 0) {
        effective_query_end = safe_subtract_gap(query_end, params.max_gap);
    }

    const auto find_first_child = [&](const Index_ children_start, const Index_ children_end) -> Index_ {
        const auto ebegin = subject.ends.begin();
        const auto estart = ebegin + children_start; 
        const auto eend = ebegin + children_end;
        return std::lower_bound(estart, eend, effective_query_end) - ebegin;
    };

    const auto is_finished = [&](const Position_ subject_start) -> bool {
        if (subject_start > query_start) {
            if (params.max_gap > 0) {
                if (subject_start - query_start > params.max_gap) {
                    return true;
                }
            } else {
                return true;
            }

            if (params.min_overlap > 0) {
                if (subject_start >= query_end || query_end - subject_start < params.min_overlap) {
                    return true;
                }
            }
        } else {
            if (params.min_overlap > 0) {
                // if query_start >= subject_start, then query_end >=
                // subject_start as well, so the LHS will be non-negative.
                if (query_end - subject_start < params.min_overlap) {
                    return true;
                }
            }
        }

        return false;
    };

    Index_ root_child_at = find_first_child(0, subject.root_children);

    workspace.history.clear();
    while (1) {
        Index_ current_subject;
        if (workspace.history.empty()) {
            if (root_child_at == subject.root_children || is_finished(subject.starts[root_child_at])) {
                break;
            }
            current_subject = root_child_at;
            ++root_child_at;
        } else {
            auto& current_state = workspace.history.back();
            if (current_state.child_at == current_state.child_end || is_finished(subject.starts[current_state.child_at])) {
                workspace.history.pop_back();
                continue;
            }
            current_subject = current_state.child_at;
            ++(current_state.child_at); // do this before the emplace_back(), otherwise the history might get reallocated and the reference would be dangling.
        }

        const auto& current_node = subject.nodes[current_subject];
        const auto subject_start = subject.starts[current_subject];
        const auto subject_end = subject.ends[current_subject];

        if (params.min_overlap > 0) {
            const auto common_end = std::min(subject_end, query_end);
            const auto common_start = std::max(subject_start, query_start);
            if (common_end <= common_start || common_end - common_start < params.min_overlap) {
                // No point processing the children if the minimum overlap isn't satisified.
                continue;
            }
        }

        // Even if the current subject interval isn't a match, its children might still be okay, so we have to keep going.
        bool okay = true;
        if (params.max_gap > 0) {
            okay = !diff_above_gap(query_start, subject_start, params.max_gap) && !diff_above_gap(query_end, subject_end, params.max_gap);
        } else {
            okay = (subject_start == query_start && subject_end == query_end);
        }

        if (okay) {
            matches.push_back(current_node.id);
            if (params.quit_on_first) {
                return;
            }
            if (current_node.duplicates_start != current_node.duplicates_end) {
                matches.insert(matches.end(), subject.duplicates.begin() + current_node.duplicates_start, subject.duplicates.begin() + current_node.duplicates_end);
            }
            if (params.max_gap == 0) { // no need to continue traversal, there should only be one node that is exactly equal.
                return;
            }
        }

        if (current_node.children_start != current_node.children_end) {
            const Index_ start_pos = find_first_child(current_node.children_start, current_node.children_end);
            if (start_pos != current_node.children_end) {
                workspace.history.emplace_back(start_pos, current_node.children_end);
            }
        }
    }
}

}

#endif

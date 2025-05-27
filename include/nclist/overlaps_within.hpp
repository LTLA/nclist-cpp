#ifndef NCLIST_OVERLAPS_WITHIN_HPP
#define NCLIST_OVERLAPS_WITHIN_HPP

#include <vector>
#include <algorithm>
#include <optional>

#include "build.hpp"

/**
 * @file overlaps_within.hpp
 * @brief Find subject ranges in which the query lies within. 
 */

namespace nclist {

/**
 * @brief Workspace for `overlaps_within()`.
 *
 * @tparam Index_ Integer type of the subject range index.
 *
 * This holds intermediate data structures that can be re-used across multiple calls to `overlaps_within()` to avoid reallocations.
 */
template<typename Index_>
struct OverlapsWithinWorkspace {
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
 * @brief Parameters for `overlaps_within()`.
 * @tparam Position_ Numeric type for the start/end positions of each range.
 */
template<typename Position_>
struct OverlapsWithinParameters {
    /**
     * Maximum difference between the lengths of the query and subject ranges.
     * An overlap is not reported between a query/subject pair if the difference is greater than `max_gap`.
     * If no value is set, the difference in lengths is not considered when reporting overlaps.
     */
    std::optional<Position_> max_gap; // can't default to -1 as Position_ might be unsigned.

    /**
     * Minimum overlap between query and subject ranges.
     * An overlap will not be reported if the length of the overlapping subrange is less than `min_overlap`.
     */
    Position_ min_overlap = 0;

    /**
     * Whether to quit immediately upon identifying an overlap with the query range.
     * In such cases, `matches` will contain one arbitrarily chosen subject range that overlaps with the query.
     */
    bool quit_on_first = false;
};

/**
 * Find subject ranges where the query range lies within them, i.e., the query is a subrange of each subject range. 
 *
 * @tparam Index_ Integer type of the subject range index.
 * @tparam Position_ Numeric type for the start/end positions of each range.
 *
 * @param subject An `Nclist` of subject ranges, typically built with `build()`. 
 * @param query_start Start of the query range.
 * @param query_end Non-inclusive end of the query range.
 * @param params Parameters for the search.
 * @param workspace Workspace for intermediate data structures.
 * This can be default-constructed and re-used across `overlaps_within()` calls.
 * @param[out] matches On output, vector of subject range indices that overlap with the query range.
 * Indices are reported in arbitrary order.
 */
template<typename Index_, typename Position_>
void overlaps_within(
    const Nclist<Index_, Position_>& subject,
    Position_ query_start,
    Position_ query_end,
    const OverlapsWithinParameters<Position_>& params,
    OverlapsWithinWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    matches.clear();
    if (subject.root_children == 0) {
        return;
    }

    /****************************************
     * # Default
     *
     * Our aim is to find overlaps to a subject interval `i` where `subject_starts[i] <= query_start` and `subject_ends[i] >= query_end`. 
     *
     * At each node of the NCList, we search for the first child where the `subject_ends` is greater than or equal to `query_end`. 
     * Earlier "sibling" intervals must have earlier end positions that must be less than that of the query interval, as well as their children, and so can be skipped.
     * We do so using a binary search (std::lower_bound) on `subject_ends` - recall that these are sorted for children of each node.
     *
     * We then iterate until the first interval `j` where `query_start < subject_starts[j]`, at which point we stop.
     * None of `j`, the children of `j`, nor any sibling intervals after `j` can have a start position less than or equal to that of the query interval, so there's no point in traversing those nodes. 
     * All subject intervals encountered during this iteration are reported in `matches`, and their children are searched in the same manner.
     *
     * Unlike `overlaps_any()`, there is no ability to skip the binary search for the descendents of a node.
     * Sure, the start positions of all descendents are no less than the node's subject interval's start position,
     * but this doesn't say much about the comparison between the subject and query end positions.
     *
     * # Max gap
     *
     * Here, our aim is the same as in the default case, with the extra requirement that the difference in lengths of overlapping subject/query pairs is less than or equal to `max_gap`.
     * This follows the same logic as in the default case with some adjustments:
     *
     * - We do not report a subject interval where the query width is greater than the subject's width by more than `max_gap`.
     *   However, we still traverse the children, as they are smaller and might satisfy the `max_gap` criterion.
     *
     * # Min overlap
     *
     * Here, we apply the extra restriction that the overlapping subinterval must have length greater than `min_overlap`.
     * This is as simple as returning early if the query interval is of length less than `min_overlap`.
     * Otherwise, any overlap that satisfies the default case will automatically have an overlapping subinterval of at least `min_overlap`.
     *
     ****************************************/

    Position_ query_width = query_end - query_start;
    if (params.min_overlap > 0 && query_width < params.min_overlap) {
        return;
    }

    auto find_first_child = [&](Index_ children_start, Index_ children_end) -> Index_ {
        auto ebegin = subject.ends.begin();
        auto estart = ebegin + children_start; 
        auto eend = ebegin + children_end;
        return std::lower_bound(estart, eend, query_end) - ebegin;
    };

    auto is_finished = [&](Position_ subject_start) -> bool {
        return subject_start > query_start;
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

        // If max_gap is violated, we don't bother to add the current subject interval,
        // but the children could be okay so we proceed to the next level of the NClist.
        bool add_self = true; 
        if (params.max_gap.has_value()) {
            auto subject_start = subject.starts[current_subject];
            auto subject_end = subject.ends[current_subject];
            auto subject_width = subject_end - subject_start;
            if (subject_width - query_width > *(params.max_gap)) {
                add_self = false;
            }
        }

        if (add_self) {
            matches.push_back(current_node.id);
            if (params.quit_on_first) {
                return;
            }
            if (current_node.duplicates_start != current_node.duplicates_end) {
                matches.insert(matches.end(), subject.duplicates.begin() + current_node.duplicates_start, subject.duplicates.begin() + current_node.duplicates_end);
            }
        }

        if (current_node.children_start != current_node.children_end) {
            Index_ start_pos = find_first_child(current_node.children_start, current_node.children_end);
            if (start_pos != current_node.children_end) {
                workspace.history.emplace_back(start_pos, current_node.children_end);
            }
        }
    }
}

}

#endif

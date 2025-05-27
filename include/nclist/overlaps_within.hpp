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

    // This is the only place that we need to consider the min_overlap; if an
    // subject interval envelops the query, it should be at least as wide, so
    // will automatically satisfy min_overlap. 
    Position_ query_width = query_end - query_start;
    if (params.min_overlap > 0 && query_width < params.min_overlap) {
        return;
    }

    // If a subject interval doesn't satisfy these requirements, none of its
    // children will either, so we can safely declare that iteration as being
    // finished. Don't check the max_gap constraints here, as parent could fail
    // this while its child could satisfy it.
    auto is_finished = [&](Position_ subject_start) -> bool {
        return subject_start > query_start;
    };

    // We start from the first subject interval that finishes at or after the query. 
    auto find_first_child = [&](Index_ children_start, Index_ children_end) -> Index_ {
        auto ebegin = subject.ends.begin();
        auto estart = ebegin + children_start; 
        auto eend = ebegin + children_end;
        if (query_width == 0) {
            // Unless the query is zero-width, in which case we're not interested in subjects that have the same endpoint.
            return std::upper_bound(estart, eend, query_end) - ebegin;
        } else {
            return std::lower_bound(estart, eend, query_end) - ebegin;
        }
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

#ifndef NCLIST_OVERLAPS_EXTEND_HPP
#define NCLIST_OVERLAPS_EXTEND_HPP

#include <vector>
#include <algorithm>
#include <optional>
#include <limits>

#include "build.hpp"

/**
 * @file overlaps_extend.hpp
 * @brief Find subject ranges that are extended by the query.
 */

namespace nclist {

/**
 * @brief Workspace for `overlaps_extend()`.
 *
 * @tparam Index_ Integer type of the subject range index.
 *
 * This holds intermediate data structures that can be re-used across multiple calls to `overlaps_extend()` to avoid reallocations.
 */
template<typename Index_>
struct OverlapsExtendWorkspace {
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
 * @brief Parameters for `overlaps_extend()`.
 * @tparam Position_ Numeric type for the start/end positions of each range.
 */
template<typename Position_>
struct OverlapsExtendParameters {
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
 * @cond
 */
template<bool has_min_overlap_, typename Index_, typename Position_>
void overlaps_extend_internal(
    const Nclist<Index_, Position_>& subject,
    Position_ query_start,
    Position_ query_end,
    const OverlapsExtendParameters<Position_>& params,
    OverlapsExtendWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    matches.clear();
    if (subject.root_children == 0) {
        return;
    }
    Position_ query_width = query_end - query_start;
    if constexpr(has_min_overlap_) {
        if (query_width < params.min_overlap) {
            return;
        }
    }

    // If a subject interval doesn't satisfy these requirements, none of its
    // children will either, so we can safely declare that iteration as being
    // finished. Don't check the max_gap constraints here, as parent could fail
    // this while its child could satisfy it.
    auto is_finished = [&](Position_ subject_start) -> bool {
        if constexpr(has_min_overlap_) {
            if (subject_start >= query_end) {
                return true;
            }
            return query_end - subject_start < params.min_overlap;
        } else {
            return subject_start >= query_end; 
        }
    };

    typename std::conditional<has_min_overlap_, Position_, const char*>::type effective_query_start; // make sure compiler complains if we use this in basic mode.
    if constexpr(has_min_overlap_) {
        // When a minimum overlap is specified, we push the query start to the right
        // so as to exclude subject intervals that end before the overlap is satisfied.
        constexpr Position_ maxed = std::numeric_limits<Position_>::max();
        if (maxed - params.min_overlap < query_start) {
            // No point continuing as nothing will be found in the binary search.
            return;
        } else {
            effective_query_start = query_start + params.min_overlap;
        }
    }

    auto find_first_child = [&](Index_ children_start, Index_ children_end) -> Index_ {
        auto ebegin = subject.ends.begin();
        auto estart = ebegin + children_start; 
        auto eend = ebegin + children_end;

        if constexpr(has_min_overlap_) {
            // For minimum overlap searches, the overlap-added start represents
            // the earliest subject end that satisfies the overlap. Now that
            // we're comparing ends to ends, we can use a lower bound.
            return std::lower_bound(estart, eend, effective_query_start) - ebegin;
        } else {
            // We use an upper bound as the interval ends are assumed to be non-inclusive,
            // so we want the first subject end that is greater than the query_start.
            return std::upper_bound(estart, eend, query_start) - ebegin;
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
        auto subject_start = subject.starts[current_subject];
        auto subject_end = subject.ends[current_subject];
        auto subject_width = subject_end - subject_start;

        if constexpr(has_min_overlap_) {
            if (subject_width < params.min_overlap) {
                // If the subject interval is smaller than the overlap, all
                // of its children will also be smaller, so we skip processing.
                continue;
            }
        }
        if (params.max_gap.has_value()) {
            if (query_width - subject_width > *(params.max_gap)) {
                // If max_gap is violated for the subject interval, all of
                // its children will also be smaller, so we can skip.
                continue;
            }
        }

        // If the subject interval ends after the query, the latter does not extend the
        // former, but its children might still be okay, so we keep going.
        bool enclosed = (query_start <= subject_start && query_end >= subject_end);
        if (enclosed) {
            matches.push_back(current_node.id);
            if (params.quit_on_first) {
                return;
            }
            if (current_node.duplicates_start != current_node.duplicates_end) {
                matches.insert(matches.end(), subject.duplicates.begin() + current_node.duplicates_start, subject.duplicates.begin() + current_node.duplicates_end);
            }
        }

        if (current_node.children_start != current_node.children_end) {
            if (enclosed) {
                // Skip the binary search if the parent was enclosed, all of its children must be similarly enclosed.
                workspace.history.emplace_back(current_node.children_start, current_node.children_end);
            } else {
                Index_ start_pos = find_first_child(current_node.children_start, current_node.children_end);
                if (start_pos != current_node.children_end) {
                    workspace.history.emplace_back(start_pos, current_node.children_end);
                }
            }
        }
    }
}
/**
 * @endcond
 */

/**
 * Find subject ranges that are extended by the query range, i.e., each subject range is a subrange of the query.
 *
 * @tparam Index_ Integer type of the subject range index.
 * @tparam Position_ Numeric type for the start/end positions of each range.
 *
 * @param subject An `Nclist` of subject ranges, typically built with `build()`. 
 * @param query_start Start of the query range.
 * @param query_end Non-inclusive end of the query range.
 * @param params Parameters for the search.
 * @param workspace Workspace for intermediate data structures.
 * This can be default-constructed and re-used across `overlaps_extend()` calls.
 * @param[out] matches On output, vector of subject range indices that overlap with the query range.
 * Indices are reported in arbitrary order.
 */
template<typename Index_, typename Position_>
void overlaps_extend(
    const Nclist<Index_, Position_>& subject,
    Position_ query_start,
    Position_ query_end,
    const OverlapsExtendParameters<Position_>& params,
    OverlapsExtendWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    if (params.min_overlap > 0) {
        overlaps_extend_internal<true>(subject, query_start, query_end, params, workspace, matches);
    } else {
        overlaps_extend_internal<false>(subject, query_start, query_end, params, workspace, matches);
    }
}

}

#endif

#ifndef NCLIST_OVERLAPS_ANY_HPP
#define NCLIST_OVERLAPS_ANY_HPP

#include <vector>
#include <algorithm>
#include <optional>
#include <limits>

#include "build.hpp"
#include "utils.hpp"

/**
 * @file overlaps_any.hpp
 * @brief Find any overlaps between ranges.
 */

namespace nclist {

/**
 * @brief Workspace for `overlaps_any()`.
 *
 * @tparam Index_ Integer type of the subject range index.
 *
 * This holds intermediate data structures that can be re-used across multiple calls to `overlaps_any()` to avoid reallocations.
 */
template<typename Index_>
struct OverlapsAnyWorkspace {
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
 * @brief Parameters for `overlaps_any()`.
 * @tparam Position_ Numeric type for the start/end positions of each range.
 */
template<typename Position_>
struct OverlapsAnyParameters {
    /**
     * Maximum gap between query and subject ranges.
     * If the gap between a query/subject pair is less than or equal to `max_gap`, the subject will be reported in `matches` even if it does not overlap with the query.
     * For example, if `max_gap = 0`, a subject that is exactly contiguous with the query will still be reported. 
     * If `max_gap` has no value, only overlapping subjects will be reported.
     * This is ignored if `min_overlap` is specified.
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
// We template by overlap mode for compile-time distinction between the
// different choices, somewhat for efficiency but also to allow us to do
// compile-time checks for invalid operations that span multiple modes.
enum class OverlapsAnyMode : char { BASIC, MIN_OVERLAP, MAX_GAP };

template<OverlapsAnyMode mode_, typename Index_, typename Position_>
void overlaps_any_internal(
    const Nclist<Index_, Position_>& subject,
    Position_ query_start,
    Position_ query_end,
    const OverlapsAnyParameters<Position_>& params,
    OverlapsAnyWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    matches.clear();
    if (subject.root_children == 0) {
        return;
    }
    if constexpr(mode_ == OverlapsAnyMode::MIN_OVERLAP) {
        if (query_end - query_start < params.min_overlap) {
            return;
        }
    }

    // If an subject interval doesn't satisfy these requirements, none of its
    // children will either, so we can safely declare that iteration is
    // finished. This allows us to skip a section of the NCList.
    auto is_finished = [&](Position_ subject_start) -> bool {
        if constexpr(mode_ == OverlapsAnyMode::MAX_GAP) {
            if (subject_start < query_end) {
                return false;
            }
            return subject_start - query_end > *(params.max_gap); // not >=, as a gap-extended end is inclusive to the starts.
        } else if constexpr(mode_ == OverlapsAnyMode::MIN_OVERLAP) {
            if (subject_start >= query_end) {
                return true;
            }
            return query_end - subject_start < params.min_overlap;
        } else {
            return subject_start >= query_end;
        }
    };

    typename std::conditional<mode_ == OverlapsAnyMode::BASIC, const char*, Position_>::type effective_query_start; // make sure compiler complains if we use this in basic mode.
    if constexpr(mode_ == OverlapsAnyMode::MAX_GAP) {
        // When a max gap is specified, we push the query start to the left
        // so as to capture subject intervals within the specified gap range.
        effective_query_start = safe_subtract_gap(query_start, *(params.max_gap));
    } else if constexpr(mode_ == OverlapsAnyMode::MIN_OVERLAP) {
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

    // The logic is that, if a query start precedes the subject start,
    // the binary search is unnecessary as we always have to start at the first child
    // (as subject_end >= subject_start >(=) query_start). Not only that, but
    // the binary search can also be skipped for all of the grandchildren's
    // children, which must necessarily start at or after the child.
    auto is_query_preceding = [&](Position_ subject_start) -> bool {
        if constexpr(mode_ == OverlapsAnyMode::BASIC) {
            return subject_start > query_start;
        } else {
            return subject_start >= effective_query_start;
        }
    };

    auto find_first_child = [&](Index_ children_start, Index_ children_end) -> Index_ {
        auto ebegin = subject.ends.begin();
        auto estart = ebegin + children_start; 
        auto eend = ebegin + children_end;
        if constexpr(mode_ == OverlapsAnyMode::BASIC) {
            // We use an upper bound as the interval ends are assumed to be non-inclusive,
            // so we want the first subject end that is greater than the query_start.
            return std::upper_bound(estart, eend, query_start) - ebegin;
        } else {
            // For gapped searches, we use a lower bound as the gap-subtracted
            // start is inclusive with the interval ends. Consider a gap of
            // zero; a query that is exactly contiguous with a subject interval
            // will be considered as overlapping that interval.
            // 
            // For minimum overlap searches, the overlap-added start represents
            // the earliest subject end that satisfies the overlap. Now that
            // we're comparing ends to ends, we can use a lower bound.
            return std::lower_bound(estart, eend, effective_query_start) - ebegin;
        }
    };

    Index_ root_child_at = 0;
    bool root_skip_search = is_query_preceding(subject.starts[0]);
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
        if constexpr(mode_ == OverlapsAnyMode::MIN_OVERLAP) {
            if (std::min(query_end, subject.ends[current_subject]) - std::max(query_start, subject.starts[current_subject]) < params.min_overlap) {
                // No point continuing with the children, as all children will by definition have smaller overlaps and cannot satisfy 'min_overlap'.
                continue;
            }
        }

        matches.push_back(current_node.id);
        if (params.quit_on_first) {
            return;
        }
        if (current_node.duplicates_start != current_node.duplicates_end) {
            matches.insert(matches.end(), subject.duplicates.begin() + current_node.duplicates_start, subject.duplicates.begin() + current_node.duplicates_end);
        }

        if (current_node.children_start != current_node.children_end) {
            if (skip_search) {
                workspace.history.emplace_back(current_node.children_start, current_node.children_end, true);
            } else {
                Index_ start_pos = find_first_child(current_node.children_start, current_node.children_end);
                if (start_pos != current_node.children_end) {
                    workspace.history.emplace_back(start_pos, current_node.children_end, is_query_preceding(subject.starts[start_pos]));
                }
            }
        }
    }
}
/**
 * @endcond
 */

/**
 * Find subject ranges that exhibit any overlap with the query range. 
 *
 * @tparam Index_ Integer type of the subject range index.
 * @tparam Position_ Numeric type for the start/end positions of each range.
 *
 * @param subject An `Nclist` of subject ranges, typically built with `build()`. 
 * @param query_start Start of the query range.
 * @param query_end Non-inclusive end of the query range.
 * @param params Parameters for the search.
 * @param workspace Workspace for intermediate data structures.
 * This can be default-constructed and re-used across `overlaps_any()` calls.
 * @param[out] matches On output, vector of subject range indices that overlap with the query range.
 * Indices are reported in arbitrary order.
 */
template<typename Index_, typename Position_>
void overlaps_any(
    const Nclist<Index_, Position_>& subject,
    Position_ query_start,
    Position_ query_end,
    const OverlapsAnyParameters<Position_>& params,
    OverlapsAnyWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    if (params.min_overlap) {
        overlaps_any_internal<OverlapsAnyMode::MIN_OVERLAP>(subject, query_start, query_end, params, workspace, matches);
    } else if (params.max_gap.has_value()) {
        overlaps_any_internal<OverlapsAnyMode::MAX_GAP>(subject, query_start, query_end, params, workspace, matches);
    } else {
        overlaps_any_internal<OverlapsAnyMode::BASIC>(subject, query_start, query_end, params, workspace, matches);
    }
}

}

#endif

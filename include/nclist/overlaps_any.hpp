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
 * @brief Find any overlaps between intervals.
 */

namespace nclist {

/**
 * @brief Workspace for `overlaps_any()`.
 *
 * @tparam Index_ Integer type of the subject interval index.
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
 * @tparam Position_ Numeric type for the start/end positions of each interval.
 */
template<typename Position_>
struct OverlapsAnyParameters {
    /**
     * Maximum gap between query and subject intervals.
     * If the gap between a query/subject pair is less than or equal to `max_gap`, the subject will be reported in `matches` even if it does not overlap with the query.
     * For example, if `max_gap = 0`, a subject that is exactly contiguous with the query will still be reported. 
     * If `max_gap` has no value, only overlapping subjects will be reported.
     * This is ignored if `min_overlap` is specified.
     */
    std::optional<Position_> max_gap; // can't default to -1 as Position_ might be unsigned.

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
 * Find subject intervals that exhibit any overlap with the query interval. 
 *
 * @tparam Index_ Integer type of the subject interval index.
 * @tparam Position_ Numeric type for the start/end positions of each interval.
 *
 * @param subject An `Nclist` of subject intervals, typically built with `build()`. 
 * @param query_start Start of the query interval.
 * @param query_end Non-inclusive end of the query interval.
 * @param params Parameters for the search.
 * @param workspace Workspace for intermediate data structures.
 * This can be default-constructed and re-used across `overlaps_any()` calls.
 * @param[out] matches On output, vector of subject interval indices that overlap with the query interval.
 * Indices are reported in arbitrary order.
 */
template<typename Index_, typename Position_>
void overlaps_any(
    const Nclist<Index_, Position_>& subject,
    const Position_ query_start,
    const Position_ query_end,
    const OverlapsAnyParameters<Position_>& params,
    OverlapsAnyWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    matches.clear();
    if (subject.root_children == 0) {
        return;
    }

    enum class OverlapsAnyMode : char { BASIC, MIN_OVERLAP, MAX_GAP };
    OverlapsAnyMode mode = OverlapsAnyMode::BASIC;
    if (params.min_overlap > 0) {
        mode = OverlapsAnyMode::MIN_OVERLAP;
    } else if (params.max_gap.has_value()) {
        mode = OverlapsAnyMode::MAX_GAP;
    }

    /****************************************
     * # Default
     *
     * Our aim is to find overlaps to a subject interval `i` where `subject_starts[i] < query_end` and `query_start < subject_ends[i]`.
     *
     * At each node of the NCList, we search for the first child where the `subject_ends` is greater than `query_start` (as ends are non-inclusive).
     * Earlier "sibling" intervals cannot overlap with the query interval, nor can their children.
     * We do so using a binary search (std::upper_bound) on `subject_ends` - recall that these are sorted for children of each node.
     *
     * We then iterate until the first interval `j` where `query_end < subject_starts[j]`, at which point we stop.
     * This is because `j`, the children of `j`, nor any sibling intervals after `j` can overlap with the query interval, so there's no point in traversing those nodes. 
     * Any subject interval encountered during this iteration is reported as an overlap, and we process its children in the same manner.
     *
     * For a modest efficiency boost, we consider the case where `query_start < subject_starts[i]` for node `i`.
     * In such cases, `subject_ends` for all children of `i` must also satisfy `query_start < subject_ends[i]`, in which case the binary search can be skipped.
     * Moreover, all descendent intervals of `i` must end after `query_start`, so the binary search can be skipped for the entire lineage of the NCList.
     *
     * # Max gap
     *
     * Here, our aim is to find subject intervals where `subject_starts[i] <= query_end + max_gap` and `query_start <= subject_ends[i] + max_gap`.
     * (Yes, the `<=` is deliberate.)
     * This follows much the same logic as in the default case with some adjustments:
     *
     * - We consider an "effective" query start as `effective_query_start = query_start - max_gap`.
     * - We then perform a binary search to find the first `subject_ends` that is greater than or equal to `effective_query_start`.
     *   This is the earliest subject interval that could lie within `max_gap` of the query or could have children that could do so.
     *   The search uses `std::lower_bound()` due to the `<=` in the problem definition above.
     * - Similarly, we check if `effective_query_start <= subject_starts[i]` to determine whether to skip the binary search.
     * - We stop iteration once `subject_starts[j] - query_end > max_gap`, after which we know that all start positions of siblings/children must lie beyond `max_gap`.
     *
     * # Min overlap
     *
     * Here, we apply the extra restriction that the overlapping subinterval must have length greater than `min_overlap`.
     * This follows much the same logic as in the default case with some adjustments:
     *
     * - We consider an "effective" query start as `effective_query_start = query_start + min_overlap`.
     *   This defines the earliest entry of `subject_ends` that still could provide an overlap of at least `min_overlap`.
     * - We perform a binary search to find the first `subject_ends` that is greater than or equal to `effective_query_start`.
     *   This uses `std::lower_bound()` as the overlap-adjusted query start is inclusive with the ends.
     * - Similarly, we check if `effective_query_start <= subject_starts[i]` to determine whether to skip the binary search.
     * - We stop iteration once `query_end - subject_start < min_overlap`, after which we know that all start positions of siblings/children must have smaller overlaps.
     * - If the length of the overlapping subinterval is less than `min_overlap`, we do not report the subject interval in `matches`.
     *   Morever, we skip traversal of that node's children, as the children must be smaller and will not satisfy `min_overlap` anyway. 
     *
     * We return early if the length of the query itself is less than `min_overlap`, as no overlap will be satisfactory.
     *
     * Fortunately, `min_overlap` and `max_gap` are mutually exclusive parameters in `overlaps_any()`, so we don't have to worry about their interaction.
     *
     ****************************************/

    if (mode == OverlapsAnyMode::MIN_OVERLAP) {
        if (query_end - query_start < params.min_overlap) {
            return;
        }
    }

    Position_ effective_query_start = std::numeric_limits<Position_>::max(); // set it to something crazy so that something weird will happen if we use it in BASIC mode.
    if (mode == OverlapsAnyMode::MAX_GAP) {
        effective_query_start = safe_subtract_gap(query_start, *(params.max_gap));
    } else if (mode == OverlapsAnyMode::MIN_OVERLAP) {
        constexpr Position_ maxed = std::numeric_limits<Position_>::max();
        if (maxed - params.min_overlap < query_start) {
            // No point continuing as nothing will be found in the binary search.
            return;
        } else {
            effective_query_start = query_start + params.min_overlap;
        }
    }

    const auto find_first_child = [&](const Index_ children_start, const Index_ children_end) -> Index_ {
        const auto ebegin = subject.ends.begin();
        const auto estart = ebegin + children_start; 
        const auto eend = ebegin + children_end;
        if (mode == OverlapsAnyMode::BASIC) {
            return std::upper_bound(estart, eend, query_start) - ebegin;
        } else {
            return std::lower_bound(estart, eend, effective_query_start) - ebegin;
        }
    };

    const auto can_skip_search = [&](const Position_ subject_start) -> bool {
        if (mode == OverlapsAnyMode::BASIC) {
            return subject_start > query_start;
        } else {
            return subject_start >= effective_query_start;
        }
    };

    const auto is_finished = [&](const Position_ subject_start) -> bool {
        if (mode == OverlapsAnyMode::BASIC) {
            return subject_start >= query_end;
        } else if (mode == OverlapsAnyMode::MAX_GAP) {
            if (subject_start < query_end) {
                return false;
            }
            return subject_start - query_end > *(params.max_gap);
        } else { // i.e., mode == OverlapsAnyMode::MIN_OVERLAP
            if (subject_start >= query_end) {
                return true;
            }
            return query_end - subject_start < params.min_overlap;
        }
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
        if (mode == OverlapsAnyMode::MIN_OVERLAP) {
            if (std::min(query_end, subject.ends[current_subject]) - std::max(query_start, subject.starts[current_subject]) < params.min_overlap) {
                // No point continuing with the children, as all children will by definition have smaller overlaps and cannot satisfy `min_overlap`.
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
                const Index_ start_pos = find_first_child(current_node.children_start, current_node.children_end);
                if (start_pos != current_node.children_end) {
                    workspace.history.emplace_back(start_pos, current_node.children_end, can_skip_search(subject.starts[start_pos]));
                }
            }
        }
    }
}

}

#endif

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
    matches.clear();
    if (subject.root_children == 0) {
        return;
    }

    /****************************************
     * # Default
     *
     * Our aim is to find overlaps to a subject interval `i` where `subject_ends[i] <= query_end` and `query_start <= subject_starts[i]`.
     *
     * At each node of the NCList, we search for the first child where the `subject_ends` is greater than or equal to `query_start`. 
     * Earlier "sibling" intervals and their children must have start positions that are earlier than the query interval, and thus cannot satisfy the constraint above.
     * We do so using a binary search (std::lower_bound) on `subject_ends` - recall that these are sorted for children of each node.
     *
     * We then iterate until the first interval `j` where `query_end < subject_starts[j]`, at which point we stop.
     * None of `j`, the children of `j`, nor any sibling intervals after `j` can have an end position before `query_end`, so there's no point in traversing those nodes. 
     * Any subject interval encountered during this iteration that is enclosed by the query is reported in `matches`.
     * Regardless of whether the query encloses the subject, we repeat the search on the children of all subject intervals encountered during iteration, as one of them might be enclosed.
     *
     * For a modest efficiency boost, we consider the case where `query_start <= subject_starts[i]` for node `i`.
     * In such cases, `subject_ends` for all children of `i` must also satisfy `query_start <= subject_ends[i]`, in which case the binary search can be skipped.
     * Moreover, all descendent intervals of `i` must end after `query_start`, so the binary search can be skipped for the entire lineage of the NCList.
     *
     * # Max gap
     *
     * Here, our aim is the same as in the default case, with the extra requirement that the difference in lengths of overlapping subject/query pairs is less than or equal to `max_gap`.
     * This follows the same logic as in the default case with some adjustments:
     *
     * - We do not traverse the children of a subject interval where the query width is greater than the subject's width by more than `max_gap`.
     *   Children will only ever be smaller so there's no point traversing them.
     *
     * # Min overlap
     *
     * Here, we apply the extra restriction that the overlapping subinterval must have length greater than `min_overlap`.
     * This follows the same logic as the default case, with some modifications:
     *
     * - We consider an "effective" query start as `effective_query_start = query_start + min_overlap`.
     *   This defines the earliest entry of `subject_ends` that still could provide an overlap of at least `min_overlap`.
     * - We perform a binary search to find the first `subject_ends` that is greater than or equal to `effective_query_start`.
     * - Similarly, we check if `effective_query_start <= subject_starts[i]` to determine whether to skip the binary search.
     * - We stop iteration once `query_end - subject_starts[j] < min_overlap`, after which we know that all children cannot achieve `min_overlap`.
     * - If the length of the subject interval is less than `min_overlap`, we skip that node and all its children, as none of them will satisfy `min_overlap`. 
     *
     * We return early if the length of the query itself is less than `min_overlap`, as no overlap will be satisfactory.
     *
     * Note that we do not use an effective query end for the binary search.
     * The comparison between query/subject ends doesn't say anything about the length of the overlap subinterval.
     *
     * These modifications are orthogonal to those required when `max_gap > 0`, and so can be combined without much effort.
     *
     ****************************************/

    const Position_ query_width = query_end - query_start;
    if (params.min_overlap > 0 && query_width < params.min_overlap) {
        return;
    }

    Position_ effective_query_start = query_start;
    if (params.min_overlap > 0) {
        constexpr Position_ maxed = std::numeric_limits<Position_>::max();
        if (maxed - params.min_overlap < query_start) {
            return; // no point continuing as nothing can be greater than or equal to the overlap-adjusted start.
        } else {
            effective_query_start = query_start + params.min_overlap;
        }
    }

    const auto find_first_child = [&](const Index_ children_start, const Index_ children_end) -> Index_ {
        const auto ebegin = subject.ends.begin();
        const auto estart = ebegin + children_start; 
        const auto eend = ebegin + children_end;
        return std::lower_bound(estart, eend, effective_query_start) - ebegin;
    };

    const auto can_skip_search = [&](const Position_ subject_start) -> bool {
        return subject_start >= effective_query_start;
    };

    const auto is_finished = [&](const Position_ subject_start) -> bool {
        if (params.min_overlap > 0) {
            if (subject_start >= query_end) {
                return true;
            }
            return query_end - subject_start < params.min_overlap;
        } else {
            return subject_start > query_end; 
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
        bool current_skip_search;
        if (workspace.history.empty()) {
            if (root_child_at == subject.root_children || is_finished(subject.starts[root_child_at])) {
                break;
            }
            current_subject = root_child_at;
            current_skip_search = root_skip_search;
            ++root_child_at;
        } else {
            auto& current_state = workspace.history.back();
            if (current_state.child_at == current_state.child_end || is_finished(subject.starts[current_state.child_at])) {
                workspace.history.pop_back();
                continue;
            }
            current_subject = current_state.child_at;
            current_skip_search = current_state.skip_search;
            ++(current_state.child_at); // do this before the emplace_back(), otherwise the history might get reallocated and the reference would be dangling.
        }

        const auto& current_node = subject.nodes[current_subject];
        const auto subject_start = subject.starts[current_subject];
        const auto subject_end = subject.ends[current_subject];
        const auto subject_width = subject_end - subject_start;

        if (params.min_overlap > 0) {
            if (subject_width < params.min_overlap) {
                continue;
            }
        }
        if (params.max_gap.has_value()) {
            if (query_width - subject_width > *(params.max_gap)) {
                continue;
            }
        }

        if (query_start <= subject_start && query_end >= subject_end) {
            matches.push_back(current_node.id);
            if (params.quit_on_first) {
                return;
            }
            if (current_node.duplicates_start != current_node.duplicates_end) {
                matches.insert(matches.end(), subject.duplicates.begin() + current_node.duplicates_start, subject.duplicates.begin() + current_node.duplicates_end);
            }
        }

        if (current_node.children_start != current_node.children_end) {
            if (current_skip_search) {
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

#ifndef NCLIST_OVERLAPS_START_HPP
#define NCLIST_OVERLAPS_START_HPP

#include <vector>
#include <algorithm>
#include <optional>
#include <limits>

#include "build.hpp"
#include "utils.hpp"

/**
 * @file overlaps_start.hpp
 * @brief Find ranges with the same start position.
 */

namespace nclist {

/**
 * @brief Workspace for `overlaps_start()`.
 *
 * @tparam Index_ Integer type of the subject range index.
 *
 * This holds intermediate data structures that can be re-used across multiple calls to `overlaps_start()` to avoid reallocations.
 */
template<typename Index_>
struct OverlapsStartWorkspace {
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
 * @brief Parameters for `overlaps_start()`.
 * @tparam Position_ Numeric type for the start/end positions of each range.
 */
template<typename Position_>
struct OverlapsStartParameters {
    /**
     * Maximum gap between the starts of the query and subject ranges.
     * An overlap is reported between a query/subject pair if the gap is equal to or less than `max_gap`.
     */
    Position_ max_gap = 0;

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
 * Find subject ranges that have the same start position as the query.
 *
 * @tparam Index_ Integer type of the subject range index.
 * @tparam Position_ Numeric type for the start/end positions of each range.
 *
 * @param subject An `Nclist` of subject ranges, typically built with `build()`. 
 * @param query_start Start of the query range.
 * @param query_end Non-inclusive end of the query range.
 * @param params Parameters for the search.
 * @param workspace Workspace for intermediate data structures.
 * This can be default-constructed and re-used across `overlaps_end()` calls.
 * @param[out] matches On output, vector of subject range indices that overlap with the query range.
 * Indices are reported in arbitrary order.
 */
template<typename Index_, typename Position_>
void overlaps_start(
    const Nclist<Index_, Position_>& subject,
    Position_ query_start,
    Position_ query_end,
    const OverlapsStartParameters<Position_>& params,
    OverlapsStartWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    matches.clear();
    if (subject.root_children == 0) {
        return;
    }

    /****************************************
     * # Default
     *
     * Our aim is to find overlaps to a subject interval `i` where `subject_start[i] == query_start`. 
     *
     * At each node of the NCList, we search for the first child where the `subject_ends` is greater than or equal to `query_start`. 
     * Earlier "sibling" intervals must have earlier start positions that cannot be equal to that of the query interval - nor can their children.
     * We do so using a binary search (std::lower_bound) on `subject_ends` - recall that these are sorted for children of each node.
     *
     * We then iterate until the first interval `j` where `query_start < subject_starts[j]`, at which point we stop.
     * None of `j`, the children of `j`, nor any sibling intervals after `j` can have a start position equal to the query interval, so there's no point in traversing those nodes. 
     * Any subject interval encountered during this iteration with an equal start position to the query is reported in `matches`.
     * Regardless of whether the start position is equal, we repeat the search on the children of all subject intervals encountered during iteration, as one of them might have an equal start position.
     *
     * For a modest efficiency boost, we consider the case where `query_start <= subject_starts[i]` for node `i`.
     * In such cases, `subject_ends` for all children of `i` must also satisfy `query_start <= subject_ends[i]`, in which case the binary search can be skipped.
     * Moreover, all descendent intervals of `i` must end after `query_start`, so the binary search can be skipped for the entire lineage of the NCList.
     *
     * # Max gap
     *
     * Here, our aim is to find subject intervals where `abs(query_start - subject_starts[i]) <= max_gap`.
     * This follows much the same logic as in the default case with some adjustments:
     *
     * - We consider an "effective" query start as `effective_query_start = query_start - max_gap`.
     * - We then perform a binary search to find the first `subject_ends` that is greater than or equal to `effective_query_start`.
     *   This is the earlier subject interval that has a start position (or has children with a start position) within `max_gap` of the query start.
     * - Similarly, we check if `effective_query_start <= subject_starts[i]` to determine whether to skip the binary search.
     * - We stop iteration once `subject_starts[j] - query_start > max_gap`, after which we know that all start positions of siblings/children must lie beyond `max_gap`.
     * - Any subject interval encountered during this iteration with a start position that satisfies `max_gap` is reported in `matches`.
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
     * - If the length of the overlapping subinterval is less than `min_overlap`, we do not report the subject interval in `matches`.
     *   Additionally, we skip traversal of that node's children, as the children must be smaller and will not satisfy `min_overlap` anyway. 
     *
     * We return early if the length of the query itself is less than `min_overlap`, as no overlap will be satisfactory.
     *
     * Note that we do not use an effective query end for the binary search.
     * The comparison between query/subject ends doesn't say anything about the length of the overlap subinterval.
     *
     * These modifications are mostly orthogonal to those required when `max_gap > 0`, so can be combined without much effort. 
     * We stop iterations when either of the stopping criteria for `max_gap` or `min_overlap` are satisfied.
     * For the effective query start, the definition from `min_overlaps` will take precedence as it is more stringent, i.e., restricts more of the search space.
     *
     ****************************************/

    if (params.min_overlap > 0) {
        if (query_end - query_start < params.min_overlap) {
            return;
        }
    }

    Position_ effective_query_start = query_start;
    bool is_simple = true;
    if (params.min_overlap > 0) {
        constexpr Position_ maxed = std::numeric_limits<Position_>::max();
        if (maxed - params.min_overlap < query_start) {
            return; // No point continuing as nothing will be found in the binary search.
        }
        effective_query_start = query_start + params.min_overlap;
        is_simple = false;
    } else if (params.max_gap > 0) {
        effective_query_start = safe_subtract_gap(query_start, params.max_gap);
        is_simple = false;
    }

    auto find_first_child = [&](Index_ children_start, Index_ children_end) -> Index_ {
        auto ebegin = subject.ends.begin();
        auto estart = ebegin + children_start; 
        auto eend = ebegin + children_end;
        return std::lower_bound(estart, eend, effective_query_start) - ebegin;
    };

    auto skip_binary_search = [&](Position_ subject_start) -> bool {
        return subject_start >= effective_query_start;
    };

    auto is_finished = [&](Position_ subject_start) -> bool {
        if (subject_start > query_start) {
            if (params.max_gap == 0) {
                return true;
            }
            if (subject_start - query_start > params.max_gap) {
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

    Index_ root_child_at = 0;
    bool root_skip_search = skip_binary_search(subject.starts[0]);
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
        auto subject_start = subject.starts[current_subject];
        auto subject_end = subject.ends[current_subject];

        // Even if the current subject interval isn't a match, its children might still be okay, so we have to keep going.
        bool okay;
        if (is_simple) {
            okay = (subject_start == query_start);
        } else {
            if (params.min_overlap > 0) {
                auto common_end = std::min(subject_end, query_end);
                auto common_start = std::max(subject_start, query_start);
                if (common_end <= common_start || common_end - common_start < params.min_overlap) {
                    // No point processing the children if the minimum overlap isn't satisified.
                    continue;
                }
            }
            if (params.max_gap > 0) {
                okay = !diff_above_gap(query_start, subject_start, params.max_gap);
            } else {
                okay = (subject_start == query_start);
            }
        }

        if (okay) {
            matches.push_back(current_node.id);
            if (params.quit_on_first) {
                return;
            }
            if (current_node.duplicates_start != current_node.duplicates_end) {
                matches.insert(matches.end(), subject.duplicates.begin() + current_node.duplicates_start, subject.duplicates.begin() + current_node.duplicates_end);
            }
        }

        if (current_node.children_start != current_node.children_end) {
            if (skip_search) {
                workspace.history.emplace_back(current_node.children_start, current_node.children_end, true);
            } else {
                Index_ start_pos = find_first_child(current_node.children_start, current_node.children_end);
                if (start_pos != current_node.children_end) {
                    workspace.history.emplace_back(start_pos, current_node.children_end, skip_binary_search(subject.starts[start_pos]));
                }
            }
        }
    }
}

}

#endif

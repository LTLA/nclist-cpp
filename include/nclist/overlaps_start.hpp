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
    if (params.min_overlap > 0) {
        if (query_end - query_start < params.min_overlap) {
            return;
        }
    }

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

    auto skip_binary_search = [&](Position_ subject_start) -> bool {
        return subject_start >= effective_query_start;
    };

    auto find_first_child = [&](Index_ children_start, Index_ children_end) -> Index_ {
        auto ebegin = subject.ends.begin();
        auto estart = ebegin + children_start; 
        auto eend = ebegin + children_end;
        return std::lower_bound(estart, eend, effective_query_start) - ebegin;
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

        // Even if the current subject interval isn't a match, its children
        // might still be okay, so we have to keep going.
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

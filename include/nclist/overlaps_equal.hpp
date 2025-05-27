#ifndef NCLIST_OVERLAPS_EQUAL_HPP
#define NCLIST_OVERLAPS_EQUAL_HPP

#include <vector>
#include <algorithm>

#include "build.hpp"
#include "utils.hpp"

/**
 * @file overlaps_equal.hpp
 * @brief Find ranges with the same start and end positions.
 */

namespace nclist {

/**
 * @brief Workspace for `overlaps_equal()`.
 *
 * @tparam Index_ Integer type of the subject range index.
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
 * @tparam Position_ Numeric type for the start/end positions of each range.
 */
template<typename Position_>
struct OverlapsEqualParameters {
    /**
     * Maximum gap between the starts/ends of query and subject ranges.
     * An overlap is reported between a query/subject pair if the gap between starts and the gap between ends are both equal to or less than `max_gap`. 
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
 * Find subject ranges with the same start and end positions as the query range.
 * By default, given a subject range `[subject_start, subject_end)`, an overlap is considered if `subject_start == query_end` and `query_start == subject_end`.
 * This behavior can be tuned with parameters in `params`.
 *
 * @tparam Index_ Integer type of the subject range index.
 * @tparam Position_ Numeric type for the start/end positions of each range.
 *
 * @param subject An `Nclist` of subject ranges, typically built with `build()`. 
 * @param query_start Start of the query range.
 * @param query_end Non-inclusive end of the query range.
 * @param params Parameters for the search.
 * @param workspace Workspace for intermediate data structures.
 * This can be default-constructed and re-used across `overlaps_equal()` calls.
 * @param[out] matches On output, vector of subject range indices that overlap with the query range.
 * Indices are reported in arbitrary order.
 */
template<typename Index_, typename Position_>
void overlaps_equal(
    const Nclist<Index_, Position_>& subject,
    Position_ query_start,
    Position_ query_end,
    const OverlapsEqualParameters<Position_>& params,
    OverlapsEqualWorkspace<Index_>& workspace,
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

    // If an subject interval doesn't satisfy these requirements, none of its
    // children or younger siblings will either, so we can safely declare that
    // iteration is finished. This allows us to skip a section of the NCList.
    auto is_finished = [&](Position_ subject_start) -> bool {
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

    Position_ effective_query_end = query_end;
    if (params.max_gap > 0) {
        // When a max gap is specified, we push the query end to the left
        // so as to capture more subject intervals. 
        effective_query_end = safe_subtract_gap(query_end, params.max_gap);
    }

    auto find_first_child = [&](Index_ children_start, Index_ children_end) -> Index_ {
        auto ebegin = subject.ends.begin();
        auto estart = ebegin + children_start; 
        auto eend = ebegin + children_end;
        return std::lower_bound(estart, eend, effective_query_end) - ebegin;
    };

    Index_ root_child_at = find_first_child(0, subject.root_children);
    bool is_simple = (params.min_overlap == 0 && params.max_gap == 0);

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

        // Even if the current subject interval isn't a match, its children
        // might still be okay, so we have to keep going.
        bool okay = true;
        if (is_simple) {
            okay = (subject_start == query_start && subject_end == query_end);
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
                okay = !diff_above_gap(query_start, subject_start, params.max_gap) && !diff_above_gap(query_end, subject_end, params.max_gap);
            } else {
                okay = (subject_start == query_start && subject_end == query_end);
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
            if (is_simple) { // no need to continue traversal, there should only be one node that is exactly equal.
                return;
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

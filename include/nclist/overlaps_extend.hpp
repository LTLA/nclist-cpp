#ifndef NCLIST_OVERLAPS_EXTEND_HPP
#define NCLIST_OVERLAPS_EXTEND_HPP

#include <vector>
#include <algorithm>
#include <optional>
#include <limits>

#include "build.hpp"

namespace nclist {

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

template<typename Position_>
struct OverlapsExtendParameters {
    std::optional<Position_> max_gap; // can't default to -1 as Position_ might be unsigned.
    Position_ min_overlap = 0;
    bool quit_on_first = false;
};

/**
 * @cond
 */
template<bool has_min_overlap_, typename Index_, typename Position_>
void overlaps_extend_internal(
    const Nclist<Index_, Position_>& index,
    Position_ query_start,
    Position_ query_end,
    const OverlapsExtendParameters<Position_>& params,
    OverlapsExtendWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    matches.clear();
    if (index.root_children == 0) {
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
        auto ebegin = index.ends.begin();
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

    Index_ root_child_at = find_first_child(0, index.root_children);

    workspace.history.clear();
    while (1) {
        Index_ current_index;
        if (workspace.history.empty()) {
            if (root_child_at == index.root_children || is_finished(index.starts[root_child_at])) {
                break;
            }
            current_index = root_child_at;
            ++root_child_at;
        } else {
            auto& current_state = workspace.history.back();
            if (current_state.child_at == current_state.child_end || is_finished(index.starts[current_state.child_at])) {
                workspace.history.pop_back();
                continue;
            }
            current_index = current_state.child_at;
            ++(current_state.child_at); // do this before the emplace_back(), otherwise the history might get reallocated and the reference would be dangling.
        }

        const auto& current_node = index.nodes[current_index];
        auto subject_start = index.starts[current_index];
        auto subject_end = index.ends[current_index];
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
                matches.insert(matches.end(), index.duplicates.begin() + current_node.duplicates_start, index.duplicates.begin() + current_node.duplicates_end);
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
 * @cond
 */

template<typename Index_, typename Position_>
void overlaps_extend(
    const Nclist<Index_, Position_>& index,
    Position_ query_start,
    Position_ query_end,
    const OverlapsExtendParameters<Position_>& params,
    OverlapsExtendWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    if (params.min_overlap > 0) {
        overlaps_extend_internal<true>(index, query_start, query_end, params, workspace, matches);
    } else {
        overlaps_extend_internal<false>(index, query_start, query_end, params, workspace, matches);
    }
}

}

#endif

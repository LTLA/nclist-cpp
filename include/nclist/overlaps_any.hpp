#ifndef NCLIST_OVERLAPS_ANY_HPP
#define NCLIST_OVERLAPS_ANY_HPP

#include <vector>
#include <algorithm>
#include <type_traits>
#include <optional>

#include "build.hpp"

namespace nclist {

template<typename Index_>
struct OverlapsAnyWorkspace {
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
struct OverlapsAnyParameters {
    std::optional<Position_> max_gap; // can't default to -1 as Position_ might be unsigned.
    Position_ min_overlap = 0;
    bool quit_on_first = false;
};

/**
 * @cond
 */
template<bool gapped_, typename Index_, typename Position_>
void overlaps_any_internal(
    const Nclist<Index_, Position_>& index,
    Position_ query_start,
    Position_ query_end,
    const OverlapsAnyParameters<Position_>& params,
    OverlapsAnyWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    matches.clear();
    if (index.root_children == 0) {
        return;
    }
    if constexpr(!gapped_) {
        if (params.min_overlap > 0 && query_end - query_start < params.min_overlap) {
            return;
        }
    }

    auto is_finished = [&](Position_ index_start) -> bool {
        if constexpr(!gapped_) {
            return index_start >= query_end;
        } else {
            if (index_start < query_end) {
                return false;
            }
            return index_start - query_end > params.max_gap; // not >=, as a gap-extended end is inclusive to the starts.
        }
    };

    auto gapped_query_start = [&]{
        if constexpr(!gapped_) {
            return static_cast<char*>(NULL); // return a NULL pointer to make sure it's never used.
        } else if (std::is_unsigned<Position_>::value && query_start < *(params.max_gap)) {
            return static_cast<Position_>(0);
        } else {
            return query_start - *(params.max_gap);
        }
    }();

    // We add a function to check if the first index end precedes the
    // (effective) query start, and to skip the binary search if so. This is
    // often true when traversing through nested children; if the parent
    // skips, many of its children will also skip.
    auto precedes_query = [&](Position_ index_end) -> bool {
        if constexpr(!gapped_) {
            return index_end <= query_start;
        } else {
            return index_end < gapped_query_start;
        }
    };

    auto find_first_child = [&](Index_ children_start, Index_ children_end) -> Index_ {
        auto ebegin = index.ends.begin();
        auto estart = ebegin + children_start; 
        auto eend = ebegin + children_end;
        if constexpr(!gapped_) {
            // We use an upper bound as the interval ends are assumed to be non-inclusive,
            // so we want the first index end that is greater than the query_start.
            return std::upper_bound(estart, eend, query_start) - ebegin;
        } else {
            // Now use a lower bound as the gap-subtracted start is inclusive with the interval ends.
            // Consider a gap of zero; this means that a query that is exactly contiguous with a 
            // subject interval will be considered as overlapping that interval.
            return std::lower_bound(estart, eend, gapped_query_start) - ebegin;
        }
    };

    Index_ root_child_at = 0;
    if (precedes_query(index.ends[0])) {
        root_child_at = find_first_child(0, index.root_children);
    }

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
        if constexpr(!gapped_) {
            if (params.min_overlap > 0) {
                if (std::min(query_end, index.ends[current_index]) - std::max(query_start, index.starts[current_index]) < params.min_overlap) {
                    // No point continuing with the children, as all children will by definition have smaller overlaps and cannot satisfy 'min_overlap'.
                    continue;
                }
            }
        }

        matches.push_back(current_node.id);
        if (params.quit_on_first) {
            return;
        }
        if (current_node.duplicates_start != current_node.duplicates_end) {
            matches.insert(matches.end(), index.duplicates.begin() + current_node.duplicates_start, index.duplicates.begin() + current_node.duplicates_end);
        }

        if (current_node.children_start != current_node.children_end) {
            if (precedes_query(index.ends[current_node.children_start])) {
                // +1 to skip the first as we already know it's before our query_start.
                Index_ start_pos = find_first_child(current_node.children_start + 1, current_node.children_end);
                if (start_pos != current_node.children_end) {
                    workspace.history.emplace_back(start_pos, current_node.children_end);
                }
            } else {
                // Skip the binary search.
                workspace.history.emplace_back(current_node.children_start, current_node.children_end);
            }
        }
    }
}
/**
 * @endcond
 */

template<typename Index_, typename Position_>
void overlaps_any(
    const Nclist<Index_, Position_>& index,
    Position_ query_start,
    Position_ query_end,
    const OverlapsAnyParameters<Position_>& params,
    OverlapsAnyWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    if (params.max_gap.has_value()) {
        overlaps_any_internal<true>(index, query_start, query_end, params, workspace, matches);
    } else {
        overlaps_any_internal<false>(index, query_start, query_end, params, workspace, matches);
    }
}

}

#endif

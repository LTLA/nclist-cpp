#ifndef NCLIST_OVERLAPS_WITHIN_HPP
#define NCLIST_OVERLAPS_WITHIN_HPP

#include <vector>
#include <algorithm>
#include <type_traits>
#include <optional>

#include "build.hpp"

namespace nclist {

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

template<typename Position_>
struct OverlapsWithinParameters {
    std::optional<Position_> max_gap; // can't default to -1 as Position_ might be unsigned.
    Position_ min_overlap = 0;
    bool quit_on_first = false;
};

template<typename Index_, typename Position_>
void overlaps_within(
    const Nclist<Index_, Position_>& index,
    Position_ query_start,
    Position_ query_end,
    const OverlapsWithinParameters<Position_>& params,
    OverlapsWithinWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    matches.clear();
    if (index.root_children == 0) {
        return;
    }

    // This is the only place that we need to consider the min_overlap; if an
    // subject interval envelops the query, it should be at least as wide, so
    // will automatically satisfy min_overlap. 
    if (params.min_overlap > 0 && query_end - query_start < params.min_overlap) {
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
        auto ebegin = index.ends.begin();
        auto estart = ebegin + children_start; 
        auto eend = ebegin + children_end;
        return std::lower_bound(estart, eend, query_end) - ebegin;
    };

    Index_ root_child_at = find_first_child(0, index.root_children);

    Position_ query_width = query_end - query_start;
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

        // If max_gap is violated, we don't bother to add the current subject interval,
        // but the children could be okay so we proceed to the next level of the NClist.
        bool add_self = true;
        if (params.max_gap.has_value()) {
            auto subject_width = index.ends[current_index] - index.starts[current_index];
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
                matches.insert(matches.end(), index.duplicates.begin() + current_node.duplicates_start, index.duplicates.begin() + current_node.duplicates_end);
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

#ifndef NCLIST_OVERLAPS_EQUAL_HPP
#define NCLIST_OVERLAPS_EQUAL_HPP

#include <vector>
#include <algorithm>
#include <optional>
#include <limits>

#include "build.hpp"
#include "utils.hpp"

namespace nclist {

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

template<typename Position_>
struct OverlapsEqualParameters {
    std::optional<Position_> max_gap; // can't default to -1 as Position_ might be unsigned.
    Position_ min_overlap = 0;
    bool quit_on_first = false;
};

template<typename Index_, typename Position_>
void overlaps_equal(
    const Nclist<Index_, Position_>& index,
    Position_ query_start,
    Position_ query_end,
    const OverlapsEqualParameters<Position_>& params,
    OverlapsEqualWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    matches.clear();
    if (index.root_children == 0) {
        return;
    }
    if (params.min_overlap > 0) {
        if (query_end - query_start < params.min_overlap) {
            return;
        }
    }

    // If an subject interval doesn't satisfy these requirements, none of its
    // children will either, so we can safely declare that iteration is
    // finished. This allows us to skip a section of the NCList.
    auto is_finished = [&](Position_ subject_start) -> bool {
        if (subject_start > query_start) {
            if (params.max_gap.has_value()) {
                return subject_start - query_start > *(params.max_gap);
            } else {
                return true;
            }
        } else {
            if (params.min_overlap > 0) {
                // if query_start >= subject_start, then query_end >=
                // subject_start as well, so the LHS will be non-negative.
                return query_end - subject_start < params.min_overlap;
            } else {
                return false;
            }
        }
    };

    Position_ effective_query_end = query_end;
    if (params.max_gap.has_value()) {
        // When a max gap is specified, we push the query end to the left
        // so as to capture more subject intervals. 
        effective_query_end = safe_subtract_gap(query_end, *(params.max_gap));
    }

    auto find_first_child = [&](Index_ children_start, Index_ children_end) -> Index_ {
        auto ebegin = index.ends.begin();
        auto estart = ebegin + children_start; 
        auto eend = ebegin + children_end;
        return std::lower_bound(estart, eend, effective_query_end) - ebegin;
    };

    Index_ root_child_at = find_first_child(0, index.root_children);
    bool is_simple = (params.min_overlap == 0 && !params.max_gap.has_value());

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
            if (params.max_gap.has_value()) {
                okay = diff_above_gap(query_start, subject_start, *(params.max_gap)) || diff_above_gap(query_end, subject_end, *(params.max_gap));
            }
        }

        if (okay) {
            matches.push_back(current_node.id);
            if (params.quit_on_first) {
                return;
            }
            if (current_node.duplicates_start != current_node.duplicates_end) {
                matches.insert(matches.end(), index.duplicates.begin() + current_node.duplicates_start, index.duplicates.begin() + current_node.duplicates_end);
            }
            if (is_simple) { // no need to continue, there is only one node that is exactly equal.
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

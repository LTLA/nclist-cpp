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
    std::vector<std::pair<Index_, Index_> > history;
    /**
     * @endcond
     */
};

/**
 * @cond
 */
template<typename Index_, typename Position_>
void overlaps_any_simple(
    const Nclist<Index_, Position_>& index,
    Position_ query_start,
    Position_ query_end,
    Position_ min_overlap,
    bool quit_on_first,
    OverlapsAnyWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    matches.clear();
    if (min_overlap > 0 && query_end - query_start < min_overlap) {
        return;
    }

    // We use an upper bound as the interval ends are assumed to be non-inclusive,
    // so we want the first end that is greater than the query_start.
    Index_ root_progress = std::upper_bound(index.ends.begin(), index.ends.begin() + index.root_children, query_start) - index.ends.begin();
    workspace.history.clear();

    while (1) {
        Index_ current_index;
        if (workspace.history.empty()) {
            if (root_progress == index.root_children || index.starts[root_progress] >= query_end) {
                break;
            }
            current_index = root_progress;
            ++root_progress;
        } else {
            auto& current_state = workspace.history.back();
            if (current_state.second == index.nodes[current_state.first].children_end || index.starts[current_state.second] >= query_end) {
                workspace.history.pop_back();
                continue;
            }
            current_index = current_state.second;
            ++(current_state.second); // do this before the emplace_back(), otherwise the history might get reallocated and the reference would be dangling.
        }

        const auto& current_node = index.nodes[current_index];
        if (min_overlap > 0) {
            if (std::min(query_end, index.ends[current_index]) - std::max(query_start, index.starts[current_index]) < min_overlap) {
                // No point continuing with the children, as all children will by definition have smaller overlaps and cannot satisfy 'min_overlap'.
                continue;
            }
        }

        matches.push_back(current_node.id);
        if (quit_on_first) {
            return;
        }
        if (current_node.duplicates_start != current_node.duplicates_end) {
            matches.insert(matches.end(), index.duplicates.begin() + current_node.duplicates_start, index.duplicates.begin() + current_node.duplicates_end);
        }

        if (current_node.children_start != current_node.children_end) {
            Index_ start_pos = std::upper_bound(index.ends.begin() + current_node.children_start, index.ends.begin() + current_node.children_end, query_start) - index.ends.begin();
            if (start_pos != current_node.children_end) {
                workspace.history.emplace_back(current_index, start_pos);
            }
        }
    }
}

template<typename Index_, typename Position_>
void overlaps_any_gapped(
    const Nclist<Index_, Position_>& index,
    Position_ query_start,
    Position_ query_end,
    Position_ max_gap,
    bool quit_on_first,
    OverlapsAnyWorkspace<Index_>& workspace,
    std::vector<Index_>& matches)
{
    auto is_finished = [&](Position_ index_start) -> bool {
        if (index_start < query_end) {
            return false;
        }
        return index_start - query_end > max_gap;
    };

    auto search_query_start = query_start;
    if (std::is_unsigned<Position_>::value && search_query_start < max_gap) {
        search_query_start = 0;
    } else {
        search_query_start -= max_gap;
    }

    // Now use a lower bound as the gap-subtracted start is inclusive with the interval ends.
    // Consider a gap of zero; this means that a query that is exactly contiguous with a 
    // subject interval will be considered as overlapping that interval.
    Index_ root_progress = std::lower_bound(index.ends.begin(), index.ends.begin() + index.root_children, search_query_start) - index.ends.begin();
    workspace.history.clear();
    matches.clear();

    while (1) {
        Index_ current_index;
        if (workspace.history.empty()) {
            if (root_progress == index.root_children || is_finished(index.starts[root_progress])) {
                break;
            }
            current_index = root_progress;
            ++root_progress;
        } else {
            auto& current_state = workspace.history.back();
            if (current_state.second == index.nodes[current_state.first].children_end || is_finished(index.starts[current_state.second])) {
                workspace.history.pop_back();
                continue;
            }
            current_index = current_state.second;
            ++(current_state.second); // do this before the emplace_back(), otherwise the history might get reallocated and the reference would be dangling.
        }

        // Unlike the simple() function, we don't consider min_overlaps here, as there
        // is no point having max_gap if we need to enforce min_overlaps as well.
        const auto& current_node = index.nodes[current_index];

        matches.push_back(current_node.id);
        if (quit_on_first) {
            return;
        }
        if (current_node.duplicates_start != current_node.duplicates_end) {
            matches.insert(matches.end(), index.duplicates.begin() + current_node.duplicates_start, index.duplicates.begin() + current_node.duplicates_end);
        }

        if (current_node.children_start != current_node.children_end) {
            Index_ start_pos = std::lower_bound(index.ends.begin() + current_node.children_start, index.ends.begin() + current_node.children_end, search_query_start) - index.ends.begin();
            if (start_pos != current_node.children_end) {
                workspace.history.emplace_back(current_index, start_pos);
            }
        }
    }
}
/**
 * @endcond
 */

template<typename Position_>
struct OverlapsAnyParameters {
    std::optional<Position_> max_gap; // can't default to -1 as Position_ might be unsigned.
    Position_ min_overlap = 0;
    bool quit_on_first = false;
};

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
        overlaps_any_gapped(index, query_start, query_end, *(params.max_gap), params.quit_on_first, workspace, matches);
    } else {
        overlaps_any_simple(index, query_start, query_end, params.min_overlap, params.quit_on_first, workspace, matches);
    }
}

}

#endif

#ifndef NCLIST_BUILD_HPP
#define NCLIST_BUILD_HPP

#include <stdexcept>
#include <vector>
#include <cstddef>
#include <algorithm>
#include <numeric>
#include <limits>
#include <type_traits>

/**
 * @file build.hpp
 * @brief Build a nested containment list.
 */

namespace nclist {

/**
 * @brief Pre-built nested containment list.
 *
 * @tparam Index_ Integer type of the subject interval index.
 * @tparam Position_ Numeric type for the start/end positions of each interval.
 *
 * Instances of an `Nclist` are usually created by `build()`.
 */
template<typename Index_, typename Position_>
struct Nclist {
/**
 * @cond
 */
    // Sequence of `nodes` that are children of the root node, i.e., `nodes[i]` is a child for `i` in `[0, root_children)`.
    Index_ root_children = 0;

    struct Node {
        Node() = default;
        Node(Index_ id) : id(id) {}

        // Index of the subject interval in the user-supplied arrays. 
        Index_ id = 0;

        // Sequence of `nodes` that are children of this node, i.e., `nodes[i]` is a child for `i` in `[children_start, children_end)`.
        // Note that length of `nodes` is no greater than the number of intervals, so we can use Index_ as the indexing type.
        Index_ children_start = 0;
        Index_ children_end = 0;

        // Sequence of `duplicates` containing indices of subject intervals that are duplicates of `id`,
        // i.e., `duplicates[i]` is a duplicate interval for `i` in `[duplicates_start, duplicates_end)`.
        // Note that length of `duplicates` is no greater than the number of intervals, so we can use Index_ as the indexing type.
        Index_ duplicates_start = 0;
        Index_ duplicates_end = 0;
    };

    std::vector<Node> nodes; 

    // These are the start and end positions corresponding to Node::id,
    // i.e., `starts[i]` is equal to `subject_starts[nodes[i].id]` where `subject_starts` is the `starts` in `build()`.
    // We put them in a separate vector for better cache locality in binary searches.
    std::vector<Position_> starts, ends;

    // Concatenations of the individual `duplicates` vectors, to reduce fragmentation.
    std::vector<Index_> duplicates;
/**
 * @endcond
 */
};

/**
 * Convenient shorthand to get the type of element in an array or array-like object.
 *
 * @tparam Array_ Class with a `[` method that accepts an `Index_` and returns a value or reference.
 */
template<class Array_>
using ArrayElement = typename std::remove_const<typename std::remove_reference<decltype(std::declval<Array_>()[0])>::type>::type;

/**
 * @cond
 */
template<class Container_, typename Size_>
void safe_resize(Container_& container, Size_ size) {
    typedef decltype(container.size()) Csize;
    constexpr Csize max_csize = std::numeric_limits<Csize>::max();
    constexpr Size_ max_size = std::numeric_limits<Size_>::max();
    if constexpr(static_cast<typename std::make_unsigned<Size_>::type>(max_size) > static_cast<typename std::make_unsigned<Csize>::type>(max_csize)) {
        if (static_cast<typename std::make_unsigned<Size_>::type>(size) > static_cast<typename std::make_unsigned<Csize>::type>(max_csize)) {
            throw std::runtime_error("failed to resize container to specified size");
        }
    }
    container.resize(size);
}

template<typename Iterator_, typename Size_>
void check_safe_ptrdiff(Size_ size) {
    typedef decltype(std::declval<Iterator_>() - std::declval<Iterator_>()) Diff;
    constexpr Diff max_diff = std::numeric_limits<Diff>::max();
    constexpr Size_ max_size = std::numeric_limits<Size_>::max();
    if constexpr(static_cast<typename std::make_unsigned<Size_>::type>(max_size) > static_cast<typename std::make_unsigned<Diff>::type>(max_diff)) {
        if (static_cast<typename std::make_unsigned<Size_>::type>(size) > static_cast<typename std::make_unsigned<Diff>::type>(max_diff)) {
            throw std::runtime_error("potential integer overflow from iterator subtraction");
        }
    }
}

template<typename Index_, class StartArray_, class EndArray_>
Nclist<Index_, ArrayElement<StartArray_> > build_internal(std::vector<Index_> of_interest, const StartArray_& starts, const EndArray_& ends) {
    typedef ArrayElement<StartArray_> Position;
    static_assert(std::is_same<Position, ArrayElement<EndArray_> >::value);

    // We want to sort by increasing start but DECREASING end, so that the children sort after their parents. 
    auto cmp = [&](Index_ l, Index_ r) -> bool {
        if (starts[l] == starts[r]) {
            return ends[l] > ends[r];
        } else {
            return starts[l] < starts[r];
        }
    };
    if (!std::is_sorted(of_interest.begin(), of_interest.end(), cmp)) {
        std::sort(of_interest.begin(), of_interest.end(), cmp);
    }

    auto num_intervals = of_interest.size();
    typedef typename Nclist<Index_, Position>::Node WorkingNode;
    std::vector<WorkingNode> working_list;
    working_list.reserve(num_intervals);

    // This section deserves some explanation.
    // For each node in the list, we need to track its children.
    // This is most easily done by allocating a vector per node, but that is very time-consuming when we have millions of nodes.
    //
    // Instead, we recognize that the number of child indices is no greater than the number of intervals (as each interval must only have one parent).
    // We allocate a 'working_children' vector of length equal to the number of intervals.
    // The left side of the vector contains the already-processed child indices, while the right side contains the currently-processed indices.
    // The aim is to store all children in a single memory allocation that can be easily freed.
    //
    // At each level, we add child indices to the right side of the vector, moving in torwards the center.
    // Once we move past a level (i.e., we discard it from 'history'), we shift its indices from the right to the left, as no more children will be added.
    // We will always have space to perform the left shift as we know the upper bound on the number of child indices.
    // This move also exposes the indices of that level's parent on the right of the vector, allowing for further addition of new children to that parent.
    //
    // The simplest approach is to left shift the children of each level separately.
    // However, if we are discarding multiple levels at once, we can accumulate their associated contiguous slices of the 'working_children' vector.
    // This allows us to perform a single left shift, eliminating overhead from repeated function calls.
    //
    // A quirk of this approach is that, because we add on the right towards the center, the children are stored in reverse order of addition.
    // This requires some later work to undo this, to report the correct order for binary search.
    //
    // We use the same approach for duplicates.
    std::vector<Index_> working_children;
    safe_resize(working_children, num_intervals);
    Index_ children_used = 0, children_tmp_boundary = num_intervals;
    std::vector<Index_> working_duplicates;
    safe_resize(working_duplicates, num_intervals);
    Index_ duplicates_used = 0, duplicates_tmp_boundary = num_intervals;

    struct Level {
        Level() = default;
        Level(Index_ offset, Position end) : offset(offset), end(end) {}
        Index_ offset; // offset into `working_list`
        Position end; // storing the end coordinate for better cache locality.
        Index_ num_children = 0;
        Index_ num_duplicates = 0;
    };
    std::vector<Level> levels(1);

    Index_ num_children_to_copy = 0;
    Index_ num_duplicates_to_copy = 0;
    auto process_level = [&](const Level& curlevel) -> void {
        auto& original_node = working_list[curlevel.offset];

        if (curlevel.num_children) {
            original_node.children_start = children_used + num_children_to_copy;
            num_children_to_copy += curlevel.num_children;
            original_node.children_end = children_used + num_children_to_copy;
        }

        if (curlevel.num_duplicates) {
            original_node.duplicates_start = duplicates_used + num_duplicates_to_copy;
            num_duplicates_to_copy += curlevel.num_duplicates;
            original_node.duplicates_end = duplicates_used + num_duplicates_to_copy;
        }
    };

    auto left_shift_indices = [&]() -> void {
        if (num_children_to_copy) {
            if (children_used != children_tmp_boundary) { // protect the copy, though this should only be relevant at the end of the traversal.
                std::copy_n(working_children.begin() + children_tmp_boundary, num_children_to_copy, working_children.begin() + children_used);
            }
            children_used += num_children_to_copy;
            children_tmp_boundary += num_children_to_copy;
        }

        if (num_duplicates_to_copy) {
            if (duplicates_used != duplicates_tmp_boundary) { // protect the copy, though this should only be relevant at the end of the traversal.
                std::copy_n(working_duplicates.begin() + duplicates_tmp_boundary, num_duplicates_to_copy, working_duplicates.begin() + duplicates_used);
            }
            duplicates_used += num_duplicates_to_copy;
            duplicates_tmp_boundary += num_duplicates_to_copy;
        }
    };

    Index_ last_id = 0;
    for (const auto& curid : of_interest) {
        auto curend = ends[curid];

        if (levels.size() > 1) { // i.e., We've processed our first interval.
            auto last_end = levels.back().end;
            if (last_end < curend) { // If we're no longer nested within the previous interval, we need to back up to the root until we are nested.
                num_children_to_copy = 0;
                num_duplicates_to_copy = 0;
                do {
                    const auto& curlevel = levels.back();
                    process_level(curlevel);
                    levels.pop_back();
                } while (levels.size() > 1 && levels.back().end < curend);
                left_shift_indices();

            } else if (last_end == curend) { // Special handling of duplicate intervals.
                if (starts[curid] == starts[last_id]) { // Only accessing 'starts' if we're forced to.
                    ++(levels.back().num_duplicates);
                    --duplicates_tmp_boundary;
                    working_duplicates[duplicates_tmp_boundary] = curid;
                    continue;
                }
            }
        }

        auto used = working_list.size();
        working_list.emplace_back(curid);
        ++(levels.back().num_children);
        --children_tmp_boundary;
        working_children[children_tmp_boundary] = used;
        levels.emplace_back(used, curend);

        last_id = curid;
    }

    num_children_to_copy = 0;
    num_duplicates_to_copy = 0;
    while (levels.size() > 1) { // processing all remaining levels except for the root node, which we'll handle separately.
        const auto& curlevel = levels.back();
        process_level(curlevel);
        levels.pop_back();
    }
    left_shift_indices();

    of_interest.clear(); // freeing up some memory for more allocations in the next section.
    of_interest.shrink_to_fit();

    // We convert the `working_list` into the output format where the children's start/end coordinates are laid out contiguously.
    // This should make for an easier binary search (allowing us to use std::lower_bound and friends) and improve cache locality.
    // We do this by traversing the list in a depth-first manner and adding all children of each node to the output vectors.
    Nclist<Index_, Position> output;
    safe_resize(output.nodes, working_list.size());
    safe_resize(output.starts, working_list.size());
    safe_resize(output.ends, working_list.size());
    output.duplicates.reserve(duplicates_used);

    // We compute iterator differences to obtain an index after std::lower_bound and friends.
    // We want to ensure that the difference fits in the difference type without overflow.
    // This can be guaranteed by checking that the difference type is large enough to hold the vector's full length.
    // We only need to check output.starts as output.ends is the same type so will have the same difference type.
    // We also cast to Index_ as this gives us a chance to avoid the check at compile time,
    // given that working_list.size() <= of_interest.size() == num_intervals/num_subset.
    check_safe_ptrdiff<decltype(output.starts.begin())>(static_cast<Index_>(working_list.size()));

    struct Level2 {
        Level2() = default;
        Level2(Index_ working_at, Index_ working_start, Index_ output_offset) : working_at(working_at), working_start(working_start), output_offset(output_offset) {}
        Index_ working_at;
        Index_ working_start;
        Index_ output_offset;
    };
    std::vector<Level2> history;

    output.root_children = levels.front().num_children;
    Index_ output_children_used = output.root_children;
    history.emplace_back(working_children.size(), children_tmp_boundary, 0);

    while (1) {
        auto& current_state = history.back();
        if (current_state.working_at == current_state.working_start) {
            history.pop_back();
            if (history.empty()) {
                break;
            } else {
                continue;
            }
        }

        // Remember that we inserted children and duplicates in reverse order into their working vectors.
        // So when we copy, we do so in reverse order to cancel out the reversal, hence the decrement here.
        --(current_state.working_at);
        auto current_working_at = current_state.working_at;
        auto current_output_offset = current_state.output_offset;
        ++(current_state.output_offset); // do all modifications to current_state before the emplace_back(), otherwise the history might get reallocated and the reference would be dangling.

        auto child = working_children[current_working_at];
        auto& current = output.nodes[current_output_offset];
        current = working_list[child];

        // Starts and ends are guaranteed to be sorted for all children of a given node (after we cancel out the reversal).
        // Obviously we already sorted in order of increasing starts, and interval indices were added to each node's children in that order.
        // For the ends, this is less obvious but any end that is equal to or less than the previous end should be a child of that previous interval and should not show up here.
        output.starts[current_output_offset] = starts[current.id];
        output.ends[current_output_offset] = ends[current.id];

        auto duplicates_old_start = current.duplicates_start, duplicates_old_end = current.duplicates_end;
        if (duplicates_old_start != duplicates_old_end) {
            current.duplicates_start = output.duplicates.size();
            output.duplicates.insert(
                output.duplicates.end(),
                working_duplicates.rbegin() + (num_intervals - duplicates_old_end),
                working_duplicates.rbegin() + (num_intervals - duplicates_old_start)
            );
            current.duplicates_end = output.duplicates.size();
        }

        auto children_old_start = current.children_start, children_old_end = current.children_end;
        if (children_old_start != children_old_end) {
            current.children_start = output_children_used;
            output_children_used += children_old_end - children_old_start;
            current.children_end = output_children_used;
            history.emplace_back(children_old_end, children_old_start, current.children_start);
        }
    }

    return output;
}
/**
 * @endcond
 */

/**
 * @tparam Index_ Integer type of the subject interval index.
 * @tparam StartArray_ Class with a `[` method that accepts an `Index_` and returns the start position of the associated interval.
 * @tparam EndArray_ Class with a `[` method that accepts an `Index_` and returns the end position of the associated interval.
 * The type of position (after removing references) should be the same as that returned by `StartArray`'s `[` method`.
 *
 * @param num_subset Number of subject intervals in the subset to include in the `Nclist`.
 * @param[in] subset Pointer to an array of length equal to `num_subset`, containing the subset of subject intervals to include in the NCList.
 * @param[in] starts Array-like object containing the start positions of all subject intervals.
 * This should be addressable by any element in `[subset, subset + num_subset)`.
 * @param[in] ends Array-like object containing the end positions of all subject intervals.
 * This should be addressable by any element in `[subset, subset + num_subset)`, where the `i`-th subject interval is defined as `[starts[i], ends[i])`.
 * Note the non-inclusive nature of the end positions.
 *
 * @return A `Nclist` containing the specified subset of subject intervals.
 */
template<typename Index_, class StartArray_, class EndArray_>
Nclist<Index_, ArrayElement<StartArray_> > build_custom(Index_ num_subset, const Index_* subset, const StartArray_& starts, const EndArray_& ends) {
    std::vector<Index_> of_interest(subset, subset + num_subset);
    return build_internal(std::move(of_interest), starts, ends);
}

/**
 * @tparam Index_ Integer type of the subject interval index.
 * @tparam StartArray_ Class with a `[` method that accepts an `Index_` and returns the start position of the associated interval.
 * @tparam EndArray_ Class with a `[` method that accepts an `Index_` and returns the end position of the associated interval.
 * The type of position (after removing references and `const`-ness) should be the same as that returned by `StartArray`'s `[` method`.
 *
 * @tparam Position_ Numeric type for the start/end position of each interval.
 * @param num_intervals Number of subject intervals to include in the NCList.
 * @param[in] starts Array-like object containing the start positions of all subject intervals.
 * This should be addressable by any element in `[0, num_intervals)`.
 * @param[in] ends Array-like object containing the end positions of all subject intervals.
 * This should be addressable by any element in `[0, num_intervals)`, where the `i`-th subject interval is defined as `[starts[i], ends[i])`.
 * Note the non-inclusive nature of the end positions.
 *
 * @return A `Nclist` containing the specified subset of subject intervals.
 */
template<typename Index_, class StartArray_, class EndArray_>
Nclist<Index_, ArrayElement<StartArray_> > build_custom(Index_ num_intervals, const StartArray_& starts, const EndArray_& ends) {
    std::vector<Index_> of_interest(num_intervals);
    std::iota(of_interest.begin(), of_interest.end(), static_cast<Index_>(0));
    return build_internal(std::move(of_interest), starts, ends);
}

/**
 * @tparam Index_ Integer type of the subject interval index.
 * @tparam Position_ Numeric type for the start/end positions of each interval.
 *
 * @param num_subset Number of subject intervals in the subset to include in the `Nclist`.
 * @param[in] subset Pointer to an array of length equal to `num_subset`, containing the subset of subject intervals to include in the NCList.
 * @param[in] starts Pointer to an array containing the start positions of all subject intervals.
 * This should be long enough to be addressable by any element in `[subset, subset + num_subset)`.
 * @param[in] ends Pointer to an array containing the end positions of all subject intervals.
 * This should be long enough to be addressable by any element in `[subset, subset + num_subset)`, where the `i`-th subject interval is defined as `[starts[i], ends[i])`.
 * Note the non-inclusive nature of the end positions.
 *
 * @return A `Nclist` containing the specified subset of subject intervals.
 */
template<typename Index_, typename Position_>
Nclist<Index_, Position_> build(Index_ num_subset, const Index_* subset, const Position_* starts, const Position_* ends) {
    return build_custom<Index_>(num_subset, subset, starts, ends);
}

/**
 * @tparam Index_ Integer type of the subject interval index.
 * @tparam Position_ Numeric type for the start/end position of each interval.
 *
 * @param num_intervals Number of subject intervals to include in the NCList.
 * @param[in] starts Pointer to an array of length `num_intervals`, containing the start positions of all subject intervals.
 * @param[in] ends Pointer to an array of length `num_intervals`, containing the end positions of all subject intervals.
 * The `i`-th subject interval is defined as `[starts[i], ends[i])`. 
 * Note the non-inclusive nature of the end positions.
 *
 * @return A `Nclist` containing all subject intervals.
 */
template<typename Index_, typename Position_>
Nclist<Index_, Position_> build(Index_ num_intervals, const Position_* starts, const Position_* ends) {
    return build_custom<Index_>(num_intervals, starts, ends);
}

}

#endif

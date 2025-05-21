#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <algorithm>
#include <unordered_set>

template<typename Index_, typename Position_>
void reference_search(
    const std::vector<Position_>& query_starts,
    const std::vector<Position_>& query_ends,
    const std::vector<Position_>& subject_starts,
    const std::vector<Position_>& subject_ends,
    std::vector<std::vector<Index_> >& output)
{
    auto nquery = query_starts.size(), nsubject = subject_starts.size();
    output.resize(nquery);
    for (auto& out : output) {
        out.clear();
    }

    // Sort by, lexicographically:
    // - position
    // - whether this is a start (ends sort first so we can terminate the interval before processing starts)
    // - whether this is a query (subjects sort first so we can start the interval before processing queries with the same starting same location)
    // - index on the query/subject vectors 
    typedef std::tuple<Position_, bool, bool, Index_> Waypoint;
    std::vector<Waypoint> trace;
    trace.reserve((nquery + nsubject) * 2);

    for (decltype(nquery) q = 0; q < nquery; ++q) {
        trace.emplace_back(query_starts[q], true, true, q);
        trace.emplace_back(query_ends[q], false, true, q);
    }
    for (decltype(nsubject) q = 0; q < nsubject; ++q) {
        trace.emplace_back(subject_starts[q], true, false, q);
        trace.emplace_back(subject_ends[q], false, false, q);
    }
    std::sort(trace.begin(), trace.end());

    std::unordered_set<Index_> query_available;
    std::unordered_set<Index_> subject_available;
    for (auto wp : trace) {
        bool is_start = std::get<1>(wp);
        bool is_query = std::get<2>(wp);
        Index_ index = std::get<3>(wp);

        if (is_query) {
            if (is_start) {
                query_available.insert(index);
                output[index].insert(output[index].end(), subject_available.begin(), subject_available.end());
            } else {
                query_available.erase(index);
            }
        } else {
            if (is_start) {
                subject_available.insert(index);
                for (auto q : query_available) {
                    output[q].push_back(index);
                }
            } else {
                subject_available.erase(index);
            }
        }
    }

    for (auto& out : output) {
        std::sort(out.begin(), out.end());
    }
}

#endif

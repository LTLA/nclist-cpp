#ifndef NCLIST_UTILS_HPP
#define NCLIST_UTILS_HPP

#include <type_traits>

namespace nclist {

template<typename Position_>
Position_ safe_subtract_gap(Position_ query_start, Position_ max_gap) {
    if (std::is_unsigned<Position_>::value && query_start < max_gap) {
        return 0;
    } else {
        return query_start - max_gap;
    }
}

template<typename Position_>
bool diff_above_gap(Position_ pos1, Position_ pos2, Position_ max_gap) {
    if (pos1 > pos2) {
        return pos1 - pos2 > max_gap;
    } else {
        return pos2 - pos1 > max_gap;
    }
}

}

#endif

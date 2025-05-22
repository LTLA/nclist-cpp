#ifndef NCLIST_UTILS_HPP
#define NCLIST_UTILS_HPP

#include <type_traits>

namespace nclist {

Position_ safe_subtract_gap(Position_ query, Position_ max_gap) {
    if constexpr(std::is_unsigned<Position_>::value) {
        if (query_start < max_gap) {
            return 0;
        }
    }
    return query_start - max_gap;
}

}

#endif

#pragma once
#include <type_traits>
#include "../../member_enum.h"

namespace cobb::reflex2 {
   class member;
   class member_gap;
   class member_range;
}

namespace cobb::reflex2::impl::enumeration {
   template<typename T> concept is_valid_member_type = std::is_same_v<T, member> || std::is_same_v<T, member_gap> || std::is_same_v<T, member_range> || is_member_enum<T>;
}

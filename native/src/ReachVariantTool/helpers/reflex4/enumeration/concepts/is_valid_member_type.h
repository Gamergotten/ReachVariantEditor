#pragma once
#include <type_traits>
#include "../../member.h"

namespace cobb::reflex4::impl::enumeration {
   template<typename T> concept is_valid_member_type = std::is_same_v<T, member> || std::is_same_v<T, member_gap> || std::is_same_v<T, member_range> || is_member_enum<T> || is_member_enum_inline<T>;
}

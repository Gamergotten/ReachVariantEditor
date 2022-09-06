#pragma once
#include <type_traits>
#include "../../member.h"

namespace cobb::reflex3::impl::enumeration {
   template<typename T> concept is_named_member = std::is_same_v<T, member> || std::is_same_v<T, member_range> || is_member_enum<T>;
}

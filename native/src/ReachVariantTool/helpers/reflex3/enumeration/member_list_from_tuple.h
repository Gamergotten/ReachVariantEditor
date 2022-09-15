#pragma once
#include <array>
#include <stdexcept>
#include <tuple>
#include "helpers/polyfills/msvc/const_reference_nttp_decltype.h"
#include "helpers/string/strcmp.h"
#include "helpers/tuple/filter_values_by_type.h"
#include "helpers/tuple/indices_of_matching_types.h"
#include "helpers/tuple/index_of_matching_value.h"
#include "helpers/type_traits/is_std_tuple.h"
#include "helpers/tuple_filter.h"
#include "helpers/tuple_foreach.h"
#include "helpers/tuple_for_each_value.h"
#include "helpers/cs.h"
#include "./concepts/is_named_member.h"
#include "./concepts/is_valid_member_type.h"
#include "../member.h"
#include "./enumeration_data_forward_declare.h"
#include "./extract_underlying_type.h"
#include "./validity_checks.h"

namespace cobb::reflex3::impl::enumeration {
   template<const auto& MemberDefinitionList> requires cobb::is_std_tuple<std::decay_t<cobb::polyfills::msvc::nttp_decltype<MemberDefinitionList>>>
   struct member_list_from_tuple {
      private:
         using tuple_type = std::decay_t<decltype(MemberDefinitionList)>;
         static constexpr auto& members = MemberDefinitionList;

         static constexpr auto named_members = cobb::tuple_filter_values_by_type<[]<typename T>(){ return is_named_member<T>; }>(members);
         
      public:
         struct value_info {
            const char*  primary_name = nullptr;
            size_t       index    = 0; // type index in (named_members)
            size_t       subindex = 0;
            member_value value    = 0;
         };

         struct alternate_name {
            const char*  name     = nullptr;
            size_t       index    = 0; // type index in (named_members)
            size_t       subindex = 0;
            member_value value    = 0;
         };
         

      protected:
         static constexpr const size_t _raw_value_count = []() consteval {
            size_t count = 0;
            cobb::tuple_for_each_value(named_members, [&count]<typename Current>(const Current& item) {
               if constexpr (std::is_same_v<Current, member>) {
                  ++count;
                  return;
               }
               if constexpr (std::is_same_v<Current, member_range>) {
                  count += item.count;
                  return;
               }
               if constexpr (is_member_enum<Current>) {
                  if constexpr (Current::enumeration::value_count == 0) {
                     ++count;
                  } else {
                     count += Current::enumeration::value_count;
                  }
                  return;
               }
            });
            return count;
         }();
         static constexpr const auto _raw_value_info_list = []() consteval {
            std::array<value_info, _raw_value_count> out = {};

            member_value v = -1;
            size_t n = 0; // index in (out)
            size_t i = 0; // member index in (named_members)
            cobb::tuple_for_each_value(members, [&i, &n, &v, &out]<typename Current>(const Current& item) {
               if constexpr (!is_named_member<Current>) {
                  ++v;
                  return;
               }
               if constexpr (std::is_same_v<Current, member> || std::is_same_v<Current, member_range>) {
                  v = item.value.value_or(v + 1);
                  //
                  if constexpr (std::is_same_v<Current, member>) {
                     out[n] = {
                        .primary_name = item.name,
                        .index        = i,
                        .value        = v,
                     };
                     ++n;
                     ++i;
                  } else if constexpr (std::is_same_v<Current, member_range>) {
                     for (size_t si = 0; si < item.count; ++si) {
                        out[n] = {
                           .primary_name = item.name,
                           .index        = i,
                           .subindex     = si,
                           .value        = v,
                        };
                        ++n;
                        ++v;
                     }
                     ++i;
                     --v;
                  }
                  return;
               }
               if constexpr (is_member_enum<Current>) {
                  v = item.value.value_or(v + 1);
                  //
                  using nested = typename Current::enumeration;
                  if constexpr (nested::value_count > 0) {
                     constexpr auto  nested_span = static_cast<member_value>(nested::max_underlying_value()) - static_cast<member_value>(nested::min_underlying_value());
                     constexpr auto& nested_list = nested::all_underlying_values;
                     //
                     ++v;
                     for (size_t si = 0; si < nested_list.size(); ++si) {
                        auto member = nested_list[si];
                        //
                        out[n] = {
                           .primary_name = item.name,
                           .index        = i,
                           .subindex     = si,
                           .value        = (v + static_cast<member_value>(si) - static_cast<member_value>(nested::min_underlying_value())),
                        };
                        ++n;
                     }
                     v += nested_span;
                     v -= 1; // at the end of handling any given member, v should equal the value of the last sub-member we processed, not the value that comes after it
                  } else {
                     out[n] = {
                        .primary_name = item.name,
                        .index        = i,
                        .subindex     = 0,
                        .value        = v,
                     };
                     ++n;
                  }
                  //
                  ++i;
                  return;
               }
            });

            if (i < std::tuple_size_v<decltype(named_members)>) // assert: we should have properly stored one or more value_infos for each named member
               throw;
            if (n < out.size()) // assert: we should have filled the output array
               throw;

            return out;
         }();
         static constexpr const auto _raw_unique_value_list = cobb::array::filter<_raw_value_info_list, [](const value_info& s, size_t i) -> bool {
            if (i == 0)
               return true;
            for (size_t j = i - 1; j < i; ++j)
               if (_raw_value_info_list[j].value == s.value)
                  return false;
            return true;
         }>;
         
         static constexpr validity_check_results_t validity_check_results = {
            .members_tuple_has_only_relevant_types = (cobb::tuple_indices_of_matching_types<tuple_type,[]<typename T>() { return !is_valid_member_type<T>; }>).size() == 0,
            .names_are_not_blank =
               []() consteval -> bool {
                  for (const auto& item : _raw_value_info_list)
                     if (cobb::strcmp(item.primary_name, ""))
                        return false;
                  return true;
               }(),
            .names_are_unique = cobb::list_items_are_unique(_raw_value_info_list, [](const auto& a, const auto& b) { return cobb::strcmp(a.primary_name, b.primary_name) != 0; }),
         };
         
         static constexpr const auto _range_counts = []() consteval{
            std::array<size_t, std::tuple_size_v<decltype(named_members)>> out = {};

            size_t i = 0; // member index in (named_members)
            cobb::tuple_for_each_value(named_members, [&i, &out]<typename Current>(const Current& item) {
               if constexpr (std::is_same_v<Current, member_range>) {
                  out[i++] = item.count;
               } else {
                  out[i++] = std::numeric_limits<size_t>::max();
               }
            });
            return out;
         }();

      public:
         static constexpr const auto value_infos = []() consteval {
            auto unique = cobb::array::filter<_raw_value_info_list, [](const value_info& item, size_t i) -> bool {
               if (i == 0)
                  return true;
               for (size_t j = i - 1; j < i; ++j)
                  if (_raw_value_info_list[j].value == item.value)
                     return false;
               return true;
            }>;
            std::sort(unique.begin(), unique.end(), [](const value_info& a, const value_info& b) { return a.value < b.value; });
            return unique;
         }();
         static constexpr const auto alternate_names = []() consteval{
            constexpr size_t count = _raw_value_info_list.size() - value_infos.size();

            std::array<alternate_name, count> out = {};

            size_t i = 0;
            for (const auto& item : _raw_value_info_list) {
               bool preserved = false;
               for (const auto& unique : value_infos) {
                  if (unique.value != item.value)
                     continue;
                  if (unique.primary_name == item.primary_name || cobb::strcmp(unique.primary_name, item.primary_name) == 0) {
                     preserved = true;
                     break;
                  }
               }
               if (!preserved) {
                  out[i] = {
                     .name     = item.primary_name,
                     .index    = item.index,
                     .subindex = item.subindex,
                     .value    = item.value,
                  };
                  ++i;
               }
            }
            return out;
         }();

      public:
         static constexpr const size_t value_count = value_infos.size();

         static constexpr const auto all_names = cobb::array::map(_raw_value_info_list, [](const value_info& item) { return item.primary_name; });
         static constexpr const auto all_underlying_values = cobb::array::map(value_infos, [](const value_info& item) { return item.value; });

         static consteval const member_value max_underlying_value() requires(value_count > 0) {
            return cobb::max(all_underlying_values);
         }
         static consteval const member_value min_underlying_value() requires(value_count > 0) {
            return cobb::min(all_underlying_values);
         }

         static consteval const size_t index_of(const char* name){
            return cobb::tuple_index_of_matching_value(
               named_members,
               [name]<typename Current>(const Current& item) -> bool {
                  return cobb::strcmp(name, item.name) == 0;
               }
            );
         };
         
      public:
         static consteval bool has(const char* name) {
            for (const auto* item_name : all_names)
               if (cobb::strcmp(name, item_name) == 0)
                  return true;
            return false;
         }

         static consteval size_t member_range_count(const char* name) {
            if (!has(name))
               throw std::invalid_argument("no member with this name exists");
            return _range_counts[index_of(name)];
         };
         static consteval bool is_range_member(const char* name) {
            return member_range_count(name) != std::numeric_limits<size_t>::max();
         };

         static consteval member_value underlying_value_of(const char* name) {
            if (!has(name))
               throw std::invalid_argument("no member with this name exists");
            for (const auto& info : value_infos)
               if (cobb::strcmp(name, info.primary_name) == 0)
                  return info.value;
            for (const auto& info : alternate_names)
               if (cobb::strcmp(name, info.name) == 0)
                  return info.value;
            throw std::logic_error("unreachable");
         }
         static consteval member_value underlying_value_of(const char* name, size_t range_index) {
            if (!is_range_member(name))
               throw std::invalid_argument("this member is not a range");
            if (range_index >= member_range_count(name))
               throw std::invalid_argument("range-index out of range");
            return underlying_value_of(name) + range_index;
         }

         static constexpr const char* name_of_value(member_value v) {
            for (const auto& info : value_infos)
               if (v == info.value)
                  return info.primary_name;
            return nullptr;
         }
   };
}
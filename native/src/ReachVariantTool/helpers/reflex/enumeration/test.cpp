#include "../enumeration.h"

namespace _cobb::reflex { // please live up to your name, IntelliSense
   using namespace cobb::reflex;
}

namespace _cobb::reflex::tests::enumeration {
   namespace test_basic {
      using test_enum = ::cobb::reflex::enumeration<
         member<cobb::cs("A")>,
         member<cobb::cs("B")>,
         member<cobb::cs("C")>,
         member<cobb::cs("D"), 1234>
      >;

      static_assert(test_enum::has<cobb::cs("A")>);
      static_assert(test_enum::has<cobb::cs("XXX")> == false);

      constexpr auto all_underlying = test_enum::all_underlying_values;

      constexpr auto underlying_a = test_enum::underlying_value_of<cobb::cs("A")>;
      constexpr auto underlying_b = test_enum::underlying_value_of<cobb::cs("B")>;
      constexpr auto underlying_c = test_enum::underlying_value_of<cobb::cs("C")>;
      constexpr auto underlying_d = test_enum::underlying_value_of<cobb::cs("D")>;
   }

   namespace test_gaps {
      using test_enum = ::cobb::reflex::enumeration<
         member<cobb::cs("A")>,
         member_gap,
         member<cobb::cs("B")>,
         member_gap,
         member<cobb::cs("C")>,
         member_gap,
         member<cobb::cs("D"), 1234>
      >;

      static_assert(test_enum::has<cobb::cs("A")>);
      static_assert(test_enum::has<cobb::cs("XXX")> == false);

      constexpr auto all_underlying = test_enum::all_underlying_values;

      constexpr auto underlying_a = test_enum::underlying_value_of<cobb::cs("A")>;
      constexpr auto underlying_b = test_enum::underlying_value_of<cobb::cs("B")>;
      constexpr auto underlying_c = test_enum::underlying_value_of<cobb::cs("C")>;
      constexpr auto underlying_d = test_enum::underlying_value_of<cobb::cs("D")>;
   }

   namespace test_range {
      using test_enum = cobb::reflex::enumeration<
         member<cobb::cs("A")>,
         member_range<cobb::cs("B"), 3>,
         member<cobb::cs("C")>
      >;

      constexpr bool test = is_member_range<member_range<cobb::cs("B"), 3>>;

      static_assert(is_member_range<member_range<cobb::cs("B"), 3>>);

      static_assert(test_enum::has<cobb::cs("A")>);
      static_assert(test_enum::has<cobb::cs("B")>);
      static_assert(test_enum::has<cobb::cs("C")>);

      constexpr auto min_underlying = test_enum::min_underlying_value;
      constexpr auto max_underlying = test_enum::max_underlying_value;

      constexpr auto all_underlying = test_enum::all_underlying_values;

      constexpr auto underlying_a = test_enum::underlying_value_of<cobb::cs("A")>;
      constexpr auto underlying_b = test_enum::underlying_value_of<cobb::cs("B")>;
      constexpr auto underlying_c = test_enum::underlying_value_of<cobb::cs("C")>;

      using typeof_b = test_enum::member_type<cobb::cs("B")>;

      constexpr auto underlying_b0 = test_enum::underlying_value_of<cobb::cs("B"), 0>;
      constexpr auto underlying_b1 = test_enum::underlying_value_of<cobb::cs("B"), 1>;
      constexpr auto underlying_b2 = test_enum::underlying_value_of<cobb::cs("B"), 2>;
      //constexpr auto underlying_c2 = test_enum::underlying_value_of<cobb::cs("C"), 2>;
   }

   namespace test_nested {
      using test_enum = cobb::reflex::enumeration<
         member<cobb::cs("A")>,
         nested_enum<
            cobb::cs("B"),
            cobb::reflex::enumeration<
               member<cobb::cs("X")>,
               member<cobb::cs("Y")>,
               member<cobb::cs("Z")>
            >
         >,
         member<cobb::cs("C")>
      >;

      using typeof_b = test_enum::member_type<cobb::cs("B")>;
      static_assert(is_nested_enum<typeof_b>);

      static_assert(test_enum::has<cobb::cs("A")>);
      static_assert(test_enum::has<cobb::cs("B")>);
      static_assert(test_enum::has<cobb::cs("C")>);

      constexpr auto min_underlying = test_enum::min_underlying_value;
      constexpr auto max_underlying = test_enum::max_underlying_value;

      constexpr auto all_underlying = test_enum::all_underlying_values;

      constexpr auto underlying_a = test_enum::underlying_value_of<cobb::cs("A")>;
      constexpr auto underlying_b = test_enum::underlying_value_of<cobb::cs("B")>;
      constexpr auto underlying_c = test_enum::underlying_value_of<cobb::cs("C")>;

      static_assert(is_nested_enum<test_enum::member_type<cobb::cs("B")>>);

      constexpr auto underlying_bx = test_enum::underlying_value_of<cobb::cs("B"), cobb::cs("X")>;
      constexpr auto underlying_by = test_enum::underlying_value_of<cobb::cs("B"), cobb::cs("Y")>;
      constexpr auto underlying_bz = test_enum::underlying_value_of<cobb::cs("B"), cobb::cs("Z")>;
      //
      constexpr auto underlying_bx_by_b = test_enum::underlying_value_of<cobb::cs("B")> + test_enum::member_type<cobb::cs("B")>::enumeration::underlying_value_of<cobb::cs("X")>;
      static_assert(underlying_bx == underlying_bx_by_b);

      constexpr auto value_a = test_enum::value_of<cobb::cs("A")>;
      constexpr auto value_b = test_enum::value_of<cobb::cs("B")>;
      constexpr auto value_c = test_enum::value_of<cobb::cs("C")>;

      constexpr auto value_bx = test_enum::value_of<cobb::cs("B"), cobb::cs("X")>;
      constexpr auto value_by = test_enum::value_of<cobb::cs("B"), cobb::cs("Y")>;
      constexpr auto value_bz = test_enum::value_of<cobb::cs("B"), cobb::cs("Z")>;

      static_assert(value_bz.to_int() == underlying_bz);

      using b_enum = typeof_b::enumeration;

      constexpr auto all_underlying_b = b_enum::all_underlying_values;
      constexpr auto min_underlying_b = b_enum::min_underlying_value;
      constexpr auto max_underlying_b = b_enum::max_underlying_value;

      constexpr b_enum bx_unwrapped = b_enum::value_of<cobb::cs("X")>;

      static_assert(test_enum::has_subset<b_enum>);
      static_assert(test_enum::name_of_subset<b_enum> == cobb::cs("B"));

      constexpr auto cast_subset_to_superset = test_enum(bx_unwrapped);
      static_assert(cast_subset_to_superset == test_enum::value_of<cobb::cs("B"), cobb::cs("X")>);
      static_assert(cast_subset_to_superset == bx_unwrapped);

      constexpr auto to_string = cast_subset_to_superset.to_c_str();
   }

   namespace test_explicit_underlying_type {
      using test_enum = cobb::reflex::enumeration<
         int16_t,
         member<cobb::cs("A")>,
         member<cobb::cs("B")>,
         member<cobb::cs("C")>,
         member<cobb::cs("D"), 5678>
      >;

      using underlying = test_enum::underlying_type;

      constexpr auto all_underlying = test_enum::all_underlying_values;
   }

   namespace test_metadata {
      struct metadata {
         int id = {};
      };
      
      using test_enum = cobb::reflex::enumeration<
         member<cobb::cs("A"), undefined, metadata{ .id = 123 }>,
         member<cobb::cs("B")>,
         member<cobb::cs("C"), undefined, metadata{ .id = 456 }>,
         member<cobb::cs("D"), 5678>
      >;

      using metadata_type = test_enum::metadata_type;
      static_assert(std::is_same_v<metadata_type, test_enum::member_type<cobb::cs("A")>::metadata_type>);

      constexpr auto value = test_enum::value_of<cobb::cs("C")>;

      constexpr auto to_string = value.to_c_str();

      constexpr auto meta = value.to_metadata();
      constexpr auto id = meta->id;
   }

   namespace test_all_constraints {
      template<typename... Types> concept is_valid_specialization = requires {
         typename cobb::reflex::enumeration<Types...>;
      };
      static_assert(
         is_valid_specialization<
            member<cobb::cs("A")>,
            member<cobb::cs("B")>
         >,
         "Sanity check for is_valid_specialization concept."
      );


      static_assert(
         false == is_valid_specialization<
            void,
            void,
            void
         >,
         "Test failed: bad type params"
      );
      static_assert(
         false == is_valid_specialization<
            uint8_t,
            member<cobb::cs("A"), 1234>
         >,
         "Test failed: member with explicit value not representable in underlying type"
      );
      static_assert(
         false == is_valid_specialization<
            member<cobb::cs("A")>,
            member<cobb::cs("B")>,
            member<cobb::cs("B")>
         >,
         "Test failed: members with non-unique names"
      );
   }
}
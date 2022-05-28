#pragma once
#include <variant>
#include "helpers/cs.h"
#include "helpers/tuple_filter.h"
#include "helpers/tuple_prepend.h"
#include "helpers/tuple_transform.h"
#include "helpers/tuple_unpack.h"
#include "halo/util/refcount.h"
#include "../../operand.h"
#include "../../operand_typeinfo.h"

#include "../../variable_type.h"
#include "./register_type.h"

namespace halo::reach::megalo::operands::variables {
   namespace impl {
      class base : public operand {
         protected:
            size_t  register_set_index = 0;
         public:
            uint8_t which = 0;
            int16_t index = 0;

            virtual void read(bitreader& stream) override;
      };

      template<typename Tuple> struct extract_indexed_type_list {
         template<typename Current> struct transformer {
            using type = util::refcount_ptr<Current>;
         };

         //
         // Iterate over all of the register set metadata types. Find any that 
         // refer to indexed data, and extract the types of indexed data they 
         // refer to. Create a std::variant of util::refcount_ptrs to those 
         // types, and give the variant std::monostate as its first type.
         //
         using type = cobb::tuple_unpack_t<
            std::variant,
            cobb::tuple_prepend<
               std::monostate,
               cobb::tuple_transform<
                  transformer,
                  cobb::tuple_filter_t<
                     []<typename Current>() {
                        return !std::is_same_v<void, typename Current::indexed_data_type>;
                     },
                     Tuple
                  >
               >
            >
         >;
      };
   }

   template<
      cobb::cs Name,
      variable_type Type,
      typename RegisterSetsTuple,
      auto RegisterSetsTupleGetter
   > class base : public impl::base {
      public:
         static constexpr variable_type type = Type;
         static constexpr operand_typeinfo typeinfo = {
            .internal_name = Name.c_str(),
         };

         static constexpr RegisterSetsTuple register_sets_tuple = RegisterSetsTupleGetter();

      protected:
         using all_indexed_types = impl::extract_indexed_type_list<typename std::decay_t<RegisterSetsTuple>>;

      public:
         all_indexed_types indexed_data = {};

         virtual void read(bitreader& stream) override {
            impl::base::read(stream);
            //
            static_assert(false, "TODO: FINISH ME");
               // - call-super won't work the way we want it to
               // - we need functions to read each of the three individual fields, passing bitcounts as appropriate

            static_assert(false, "TODO: FINISH ME");
               // - get the register_set_definition by index
               //    - how do we do that if they're different types?
               // - if it's indexed data, set our (indexed_data) variant to the right pointer
               //
               // - we only need to handle indexed data at this step
               //    - so...
               //       - for-each template to generate a function table to handle it with?
               //
               // - actually, here's a better idea:
               //   
               //    - define a non-templated type `register_set_metadata`
               //   
               //    - have a templated wrapper `register_set_definition` which holds the 
               //      metadata along with template-specific info (i.e. the indexed-data 
               //      accessor)
               //   
               //    - now we can generate a constexpr list of metadata
               //   
               //       - we can access that by index, at run-time
               //   
               //       - we can also iterate it at run-time
               //   
               //    - as a bonus, instead of taking a tuple getter (to work around the 
               //      structs we're using not being structural types), maybe we can just 
               //      template the variable type on a const reference?
               //   
               //       - tested in godbolt and it works
         }
   };
}
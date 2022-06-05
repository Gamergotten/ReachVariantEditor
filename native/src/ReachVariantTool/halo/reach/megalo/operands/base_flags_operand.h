#pragma once
#include "helpers/cs.h"
#include "helpers/reflex_enum/reflex_flags.h"
#include "halo/reach/bitstreams.fwd.h"
#include "../operand_typeinfo.h"
#include "../operand.h"

namespace halo::reach::megalo::operands {
   template<cobb::cs Name, typename T> requires cobb::is_reflex_flags<T> class base_flags_operand : public operand {
      public:
         static constexpr auto name = Name;

         using value_type     = T;
         using bitnumber_type = bitnumber<
            std::bit_width(value_type::flag_count),
            value_type
         >;
         
         static constexpr operand_typeinfo typeinfo = {
            .internal_name = name.c_str(),
         };

      public:
         bitnumber_type value;

         virtual void read(bitreader& stream) override {
            stream.read(this->value);
         }
   };
}

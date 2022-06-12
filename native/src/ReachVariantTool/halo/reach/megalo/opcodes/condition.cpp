#include "condition.h"
#include "all_conditions.h"
#include "halo/reach/bitstreams.h"
#include "halo/reach/megalo/create_operand.h"

#include "halo/reach/megalo/load_process_messages/opcode/bad_function_index.h"

namespace halo::reach::megalo {
   void condition::read(bitreader& stream) {
      constexpr auto& list           = all_conditions;
      constexpr auto  index_bitcount = std::bit_width(list.size() - 1);

      size_t fi = stream.read_bits(index_bitcount);
      if (fi >= list.size()) {
         stream.throw_fatal_at<load_process_messages::megalo::opcode_bad_function_index>(
            {
               .opcode_type = opcode_type::condition,
               .function_id = fi,
            },
            stream.get_position().rewound_by_bits(index_bitcount)
         );
         return;
      }
      this->function = &list[fi];
      if (fi == 0) { // "none" opcode
         return;
      }
      stream.read(
         invert,
         or_group,
         load_state.execute_before
      );
      //
      // Load args:
      // 
      size_t count = this->function->operands.size();
      this->operands.resize(count);
      for (size_t i = 0; i < count; ++i) {
         auto&  definition = this->function->operands[i];
         auto*& entry      = this->operands[i];
         entry = create_operand(definition.typeinfo);
         assert(entry);
         stream.read(*entry);
      }
   }
}
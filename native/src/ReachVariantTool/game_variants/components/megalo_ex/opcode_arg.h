#pragma once
#include "../../base.h"
#include "../../../helpers/bitvector.h"
#include "../../../helpers/reference_tracked_object.h"
#include <functional>
#include <QString>

namespace MegaloEx {
   class Compiler;

   class OpcodeArgValue;

   enum class arg_consume_result {
      failure,
      success,
      still_hungry
   };
   class OpcodeArgTypeinfo {
      public:
         using load_functor_t        = std::function<bool(OpcodeArgValue&, uint64_t bits)>; // loads data from binary stream; returns success/failure
         using decode_functor_t      = std::function<bool(OpcodeArgValue&, std::string&)>; // returns success/failure
         using compile_functor_t     = std::function<arg_consume_result(OpcodeArgValue&, const std::string&, Compiler&)>;
         using postprocess_functor_t = std::function<bool(OpcodeArgValue&, GameVariantData*)>;
         //
         static bool default_postprocess_functor(OpcodeArgValue&, GameVariantData*) { return true; } // for argument types that don't need postprocessing
         //
         struct flags {
            flags() = delete;
            enum type : uint32_t {
               none = 0,
               may_need_postprocessing = 0x00000001,
               is_variable             = 0x00000002,
               is_nestable_variable    = 0x00000004,
               is_static_variable      = 0x00000008, // e.g. player[3], team[3]
            };
         };
         using flags_type = std::underlying_type_t<flags::type>;
         //
         QString    name;
         QString    desc;
         flags_type flags = 0;
         //
         load_functor_t        load        = nullptr;
         postprocess_functor_t postprocess = nullptr;
         decode_functor_t      to_english  = nullptr;
         decode_functor_t      decompile   = nullptr;
         compile_functor_t     compile     = nullptr;
         //
         OpcodeArgTypeinfo(
            QString name,
            QString desc,
            uint32_t f,
            load_functor_t        l,
            postprocess_functor_t p,
            decode_functor_t      e,
            decode_functor_t      d,
            compile_functor_t     c = nullptr // optional for now so that we can add these incrementally once we begin actually implementing a compiler
         )
            : name(name), desc(desc), flags(f), load(l), postprocess(p), to_english(e), decompile(d), compile(c)
         {}
   };
   
   class OpcodeArgValue : public virtual IGameVariantDataObjectNeedingPostprocess, cobb::reference_tracked_object {
      //
      // This class stores only the raw bits that represent the argument value, and a reference to a "typeinfo" 
      // object. The typeinfo object is capable of loading raw bits from a file, compiling a script argument into 
      // raw bits, and decompiling raw bits into a script argument.
      //
      public:
         virtual bool postprocess(GameVariantData* data) override { // IGameVariantDataObjectNeedingPostprocess
            return (this->type.postprocess)(*this, data);
         }
         //
         cobb::bitvector64  data;
         OpcodeArgTypeinfo& type;
         uint8_t            compile_step = 0;
         QString            english; // for debugging
         ref<cobb::reference_tracked_object> relevant_objects[4]; // for when this argument refers to something that needs to be reference-tracked, like a Forge label or script option
         //
         using bit_storage_type = decltype(data)::storage_type;
   };
}
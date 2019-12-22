#pragma once
#include "../../../helpers/bitnumber.h"
#include "../limits.h"
#include "../opcode_arg.h"
#include "object.h"
#include "player.h"
#include "scalar.h"
#include "team.h"
#include "timer.h"

namespace Megalo {
   enum class OpcodeStringTokenType : int8_t {
      none = -1,
      //
      player, // player's gamertag
      team,   // "team_none" or team designator string
      object, // "none" or "unknown"
      number, // "%i"
      // number_with_sign, // Halo 4 only // "+%i" if positive; "%i" otherwise according to KSoft.Tool source
      timer,  // "%i:%02i:%02i" or "%i:%02i" or "0:%02i" according to KSoft.Tool source
   };
   //
   class OpcodeStringToken {
      public:
         cobb::bitnumber<3, OpcodeStringTokenType, true> type = OpcodeStringTokenType::none;
         OpcodeArgValue* value = nullptr;
         //
         ~OpcodeStringToken() {
            if (this->value) {
               delete this->value;
               this->value = nullptr;
            }
         }
         //
         bool read(cobb::bitreader& stream) noexcept {
            this->type.read(stream);
            switch (this->type) {
               case OpcodeStringTokenType::none:
                  break;
               case OpcodeStringTokenType::player:
                  this->value = new OpcodeArgValuePlayer();
                  break;
               case OpcodeStringTokenType::team:
                  this->value = new OpcodeArgValueTeam();
                  break;
               case OpcodeStringTokenType::object:
                  this->value = new OpcodeArgValueObject();
                  break;
               case OpcodeStringTokenType::number:
               //case OpcodeStringTokenType::number_with_sign:
                  this->value = new OpcodeArgValueScalar();
                  break;
               case OpcodeStringTokenType::timer:
                  this->value = new OpcodeArgValueTimer();
                  break;
               default:
                  return false;
            }
            if (this->value) {
               this->value->read(stream);
            }
            return true;
         }
         void write(cobb::bitwriter& stream) const noexcept {
            this->type.write(stream);
            if (this->value)
               this->value->write(stream);
         }
         void to_string(std::string& out) const noexcept {
            if (this->value)
               this->value->to_string(out);
         }
   };

   template<int N> class OpcodeArgValueStringTokens : OpcodeArgValue {
      //
      // An opcode argument which consists of a format string and zero or more tokens to 
      // insert into it. The format string is specified as an index into the string 
      // table, and that index can contain printf-style format codes (though I don't 
      // know that the game uses printf directly as opposed to a custom format inspired 
      // by it).
      //
      // Format specifiers seen:
      //    %n    Prints a game state value (e.g. Round Limit) as a number.
      //    %s    Not yet known. "Safe Haven - %s"
      //
      // TODO: Does the game do a raw printf? If so, can we cause crashes using bad 
      //       parameters? If so, any script editor will need to validate parameters.
      //
      public:
         cobb::bitnumber<cobb::bitcount(Limits::max_variant_strings - 1), int32_t, true> stringIndex = -1; // format string - index in scriptData::strings
         cobb::bitnumber<cobb::bitcount(N), uint8_t> tokenCount = 0;
         OpcodeStringToken tokens[N];
         //
         virtual bool read(cobb::bitreader& stream) noexcept override {
            this->stringIndex.read(stream); // string table index pointer; -1 == none
            this->tokenCount.read(stream);
            if (this->tokenCount > N) {
               printf("Tokens value claimed to have %d tokens; max is %d.\n", this->tokenCount.to_int(), N);
               return false;
            }
            for (uint8_t i = 0; i < this->tokenCount; i++)
               this->tokens[i].read(stream);
            return true;
         }
         virtual void write(cobb::bitwriter& stream) const noexcept override {
            this->stringIndex.write(stream);
            this->tokenCount.write(stream);
            for (uint8_t i = 0; i < this->tokenCount; i++)
               this->tokens[i].write(stream);
         }
         virtual void to_string(std::string& out) const noexcept override {
            if (this->tokenCount == 0) {
               if (this->stringIndex == -1) {
                  out = "no string";
                  return;
               }
               cobb::sprintf(out, "localized string ID %d", this->stringIndex);
               return;
            }
            out.clear();
            for (uint8_t i = 0; i < this->tokenCount; i++) {
               std::string temp;
               //
               auto& token = this->tokens[i];
               token.to_string(temp);
               //
               if (!out.empty())
                  out += ", ";
               out += temp;
            }
            cobb::sprintf(out, "format string ID %d and tokens: %s", this->stringIndex, out.c_str());
         }
         //
         static OpcodeArgValue* factory(cobb::bitreader&) {
            return new OpcodeArgValueStringTokens;
         }
   };
   using OpcodeArgValueStringTokens1 = OpcodeArgValueStringTokens<1>;
   using OpcodeArgValueStringTokens2 = OpcodeArgValueStringTokens<2>;
   using OpcodeArgValueStringTokens3 = OpcodeArgValueStringTokens<3>;
}
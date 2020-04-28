#include "icons.h"
#include "../compiler/compiler.h"

namespace Megalo {
   namespace enums {
      extern DetailedEnum engine_icon = DetailedEnum({
         DetailedEnumValue("flag"),
         DetailedEnumValue("slayer"),
         DetailedEnumValue("oddball"),
         DetailedEnumValue("koth"),
         DetailedEnumValue("juggernaut"),
         DetailedEnumValue("territories"),
         DetailedEnumValue("assault"),
         DetailedEnumValue("infection"),
         DetailedEnumValue("vip"),
         DetailedEnumValue("invasion"),
         DetailedEnumValue("invasion_slayer"),
         DetailedEnumValue("stockpile"),
         DetailedEnumValue("action_sack"),
         DetailedEnumValue("race"),
         DetailedEnumValue("rocket_race"),
         DetailedEnumValue("grifball"),
         DetailedEnumValue("soccer"),
         DetailedEnumValue("headhunter"),
         DetailedEnumValue("crosshair"),
         DetailedEnumValue("wheel"),
         DetailedEnumValue("swirl"),
         DetailedEnumValue("bunker"),
         DetailedEnumValue("health_pack"),
         DetailedEnumValue("castle_defense"),
         DetailedEnumValue("return"),
         DetailedEnumValue("shapes"),
         DetailedEnumValue("cartographer"),
         DetailedEnumValue("eight_ball"),
         DetailedEnumValue("noble"),
         DetailedEnumValue("covenant"),
         DetailedEnumValue("attack"),
         DetailedEnumValue("defend"),
         DetailedEnumValue("ordnance"),
         DetailedEnumValue("circle"),
         DetailedEnumValue("recon"),
         DetailedEnumValue("recover"),
         DetailedEnumValue("ammo"),
         DetailedEnumValue("skull"),
         DetailedEnumValue("forge"),
         DetailedEnumValue("", DetailedEnumValueInfo::make_unused_sentinel()),
         DetailedEnumValue("", DetailedEnumValueInfo::make_unused_sentinel()),
         DetailedEnumValue("", DetailedEnumValueInfo::make_unused_sentinel()),
         DetailedEnumValue("", DetailedEnumValueInfo::make_unused_sentinel()),
         DetailedEnumValue("", DetailedEnumValueInfo::make_unused_sentinel()),
         DetailedEnumValue("", DetailedEnumValueInfo::make_unused_sentinel()),
         DetailedEnumValue("", DetailedEnumValueInfo::make_unused_sentinel()),
         DetailedEnumValue("", DetailedEnumValueInfo::make_unused_sentinel()),
         DetailedEnumValue("", DetailedEnumValueInfo::make_unused_sentinel()),
         DetailedEnumValue("", DetailedEnumValueInfo::make_unused_sentinel()),
         DetailedEnumValue("", DetailedEnumValueInfo::make_unused_sentinel()),
         DetailedEnumValue("recent_games"),
         DetailedEnumValue("file_share"),
      });
      extern DetailedEnum hud_widget_icon = DetailedEnum({
         DetailedEnumValue("flag"),
         DetailedEnumValue("slayer"),
         DetailedEnumValue("oddball"),
         DetailedEnumValue("koth"),
         DetailedEnumValue("juggernaut"),
         DetailedEnumValue("territories"),
         DetailedEnumValue("assault"),
         DetailedEnumValue("infection"),
         DetailedEnumValue("vip"),
         DetailedEnumValue("invasion"),
         DetailedEnumValue("extermination"), // TODO: verify
         DetailedEnumValue("stockpile"),
         DetailedEnumValue("action_sack"),
         DetailedEnumValue("race"),
         DetailedEnumValue("rocket_race"),
         DetailedEnumValue("grifball"),
         DetailedEnumValue("soccer"),
         DetailedEnumValue("headhunter"),
         // TODO: verify all below icons
         DetailedEnumValue("crosshair"),
         DetailedEnumValue("wheel"),
         DetailedEnumValue("swirl"),
         DetailedEnumValue("bunker"),
         DetailedEnumValue("health_pack"),
         DetailedEnumValue("castle_defense"),
         DetailedEnumValue("return"),
         DetailedEnumValue("shapes"),
         DetailedEnumValue("cartographer"),
         DetailedEnumValue("eight_ball"),
         DetailedEnumValue("noble"),
         DetailedEnumValue("covenant"),
         DetailedEnumValue("attack"),
         DetailedEnumValue("defend"),
         DetailedEnumValue("ordnance"),
         DetailedEnumValue("circle"),
         DetailedEnumValue("recon"),
         DetailedEnumValue("recover"),
         DetailedEnumValue("ammo"),
      });
   }
   
   #pragma region OpcodeArgValueIconBase
      bool OpcodeArgValueIconBase::read(cobb::ibitreader& stream, GameVariantDataMultiplayer& mp) noexcept {
         bool absence = stream.read_bits(1) != 0;
         if (absence) {
            this->value = none;
            return true;
         }
         this->value = stream.read_bits(bitcount);
         return true;
      }
      void OpcodeArgValueIconBase::write(cobb::bitwriter& stream) const noexcept {
         if (this->value == none) {
            stream.write(1, 1);
            return;
         }
         stream.write(0, 1);
         //
         stream.write(this->value, bitcount);
      }
      void OpcodeArgValueIconBase::to_string(std::string& out) const noexcept {
         if (this->value < 0) {
            out = "none";
            return;
         }
         auto item = this->icons.item(this->value);
         if (!item) {
            cobb::sprintf(out, "invalid value %u", this->value);
            return;
         }
         QString name = item->get_friendly_name();
         if (!name.isEmpty()) {
            out = name.toStdString();
            return;
         }
         out = item->name;
         if (out.empty()) // enum values should never be nameless but just in case
            cobb::sprintf(out, "%u", this->value);
      }
      void OpcodeArgValueIconBase::decompile(Decompiler& out, Decompiler::flags_t flags) noexcept {
         if (this->value < 0) {
            out.write("none");
            return;
         }
         auto item = this->icons.item(this->value);
         std::string temp;
         if (!item) {
            cobb::sprintf(temp, "%u", this->value);
            out.write(temp);
            return;
         }
         temp = item->name;
         if (temp.empty())
            cobb::sprintf(temp, "%u", this->value);
         out.write(temp);
      }
      arg_compile_result OpcodeArgValueIconBase::compile(Compiler& compiler, Script::string_scanner& arg, uint8_t part) noexcept {
         if (part > 0)
            return arg_compile_result::failure();
         //
         const int max_value = cobb::bitmax(this->bitcount);
         //
         int32_t value = 0;
         if (arg.extract_integer_literal(value)) {
            if (value < 0 || value > max_value) // do not allow incident IDs to overflow
               return arg_compile_result::failure(QString("Integer literal %1 is out of bounds; valid integers range from 0 to %2.").arg(value).arg(max_value));
            this->value = value;
            return arg_compile_result::success();
         }
         QString word = arg.extract_word();
         if (word.isEmpty())
            return arg_compile_result::failure();
         auto alias = compiler.lookup_absolute_alias(word);
         if (alias) {
            if (alias->is_integer_constant()) {
               value = alias->get_integer_constant();
               if (value < 0 || value > max_value) // do not allow IDs to overflow
                  return arg_compile_result::failure(QString("Integer literal %1 (from the alias named \"%3\") is out of bounds; valid integers range from 0 to %2.").arg(value).arg(max_value).arg(alias->name));
               this->value = value;
               return arg_compile_result::success();
            }
            if (alias->is_imported_name())
               word = alias->target_imported_name;
            else
               return arg_compile_result::failure(QString("Alias \"%1\" cannot be used here. Only integer literals, icon names, and aliases of either may appear here.").arg(alias->name));
         }
         if (word.compare("none", Qt::CaseInsensitive) == 0) {
            this->value = -1;
            return arg_compile_result::success();
         }
         value = this->icons.lookup(word);
         if (value < 0)
            return arg_compile_result::failure(QString("Value \"%1\" is not a recognized name for this type of icon.").arg(word));
         this->value = value;
         return arg_compile_result::success();
      }
      void OpcodeArgValueIconBase::copy(const OpcodeArgValue* other) noexcept {
         auto cast = dynamic_cast<const OpcodeArgValueIconBase*>(other);
         assert(cast);
         assert(&this->icons == &cast->icons && "These are different icon types.");
         assert(this->bitcount == cast->bitcount && "These are different icon types.");
         this->value = cast->value;
      }
   #pragma endregion
   //
   OpcodeArgTypeinfo OpcodeArgValueEngineIcon::typeinfo = OpcodeArgTypeinfo(
      "_engine_icon",
      "Engine Icon",
      "One of a limited set of engine-defined icons, including gametype icons.",
      //
      OpcodeArgTypeinfo::flags::none,
      OpcodeArgTypeinfo::default_factory<OpcodeArgValueEngineIcon>,
      enums::engine_icon
   ).import_names({ "none" });
   //
   OpcodeArgTypeinfo OpcodeArgValueHUDWidgetIcon::typeinfo = OpcodeArgTypeinfo(
      "_hud_widget_icon",
      "HUD Widget Icon",
      "One of a limited set of engine-defined icons, including gametype icons.",
      //
      OpcodeArgTypeinfo::flags::none,
      OpcodeArgTypeinfo::default_factory<OpcodeArgValueHUDWidgetIcon>,
      enums::hud_widget_icon
   ).import_names({ "none" });
}
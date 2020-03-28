#include "trigger.h"
#include "limits.h"
#include "opcode_arg_types/all_indices.h"
#include "../../types/multiplayer.h"

namespace {
   constexpr int ce_max_index      = Megalo::Limits::max_script_labels;
   constexpr int ce_index_bitcount = cobb::bitcount(ce_max_index); // DON'T subtract 1; you can do "for each with no label" and that encodes as -1, i.e. we need an extra bit for the sign
   //
   using _index_t = int8_t;
   static_assert(std::numeric_limits<_index_t>::max() >= ce_max_index, "Use a larger type.");
}
namespace Megalo {
   Trigger::~Trigger() {
      for (auto opcode : this->opcodes)
         delete opcode;
      this->opcodes.clear();
   }
   //
   bool Trigger::read(cobb::ibitreader& stream, GameVariantDataMultiplayer& mp) noexcept {
      #ifdef _DEBUG
         this->bit_offset = stream.get_bitpos();
      #endif
      this->blockType.read(stream);
      this->entryType.read(stream);
      if (this->blockType == block_type::for_each_object_with_label) {
         _index_t index = stream.read_bits(ce_index_bitcount);
         if (index != -1) {
            auto& list = mp.scriptContent.forgeLabels;
            if (index < list.size())
               this->forgeLabel = &list[index];
         }
      }
      this->raw.conditionStart.read(stream);
      this->raw.conditionCount.read(stream);
      this->raw.actionStart.read(stream);
      this->raw.actionCount.read(stream);
      return true;
   }
   void Trigger::postprocess_opcodes(const std::vector<Condition>& conditions, const std::vector<Action>& actions) noexcept {
      //
      // Opcodes are written to the file as flat lists: all conditions in the entire script, consecutively; then 
      // all actions in the entire script, consecutively; and then all triggers. Indices and counts in the triggers 
      // and conditions make it possible to reconstruct the triggers (and properly interleave conditions and actions) 
      // once all data is loaded.
      //
      auto& raw = this->raw;
      std::vector<Action*> temp;
      //
      this->opcodes.reserve(raw.actionCount + raw.conditionCount);
      temp.reserve(raw.actionCount);
      //
      if (raw.actionCount > 0) {
         assert(raw.actionStart < actions.size() && "Bad trigger first-action-index."); // TODO: fail with an error in-app instead of asserting
         if (raw.actionStart >= 0) {
            size_t end = raw.actionStart + raw.actionCount;
            if (end <= actions.size()) // TODO: if (end) is too high, fail with an error
               for (size_t i = raw.actionStart; i < end; i++) {
                  auto instance = new Action(actions[i]);
                  this->opcodes.push_back(instance);
                  temp.push_back(instance);
               }
         }
      }
      if (raw.conditionCount > 0) {
         assert(raw.conditionStart < conditions.size() && "Bad trigger first-conditions-index."); // TODO: fail with an error in-app instead of asserting
         if (raw.conditionStart >= 0) {
            size_t end = raw.conditionStart + raw.conditionCount;
            if (end <= conditions.size()) // TODO: if (end) is too high, fail with an error
               for (size_t i = raw.conditionStart; i < end; i++) {
                  auto& condition = conditions[i];
                  if (condition.action >= raw.actionCount) {
                     this->opcodes.push_back(new Condition(condition));
                     continue;
                  }
                  auto target = temp[condition.action];
                  auto it = std::find(this->opcodes.begin(), this->opcodes.end(), target);
                  assert(it != this->opcodes.end() && "Action not present in the opcode list, even though we have to have inserted it?!");
                  this->opcodes.insert(it, new Condition(condition));
               }
         }
      }
   }
   void Trigger::write(cobb::bitwriter& stream) const noexcept {
      this->blockType.write(stream);
      this->entryType.write(stream);
      if (this->blockType == block_type::for_each_object_with_label) {
         _index_t index = -1;
         if (this->forgeLabel)
            index = this->forgeLabel->index;
         stream.write(index, ce_index_bitcount);
      }
      this->raw.conditionStart.write(stream);
      this->raw.conditionCount.write(stream);
      this->raw.actionStart.write(stream);
      this->raw.actionCount.write(stream);
   }
   //
   void Trigger::prep_for_flat_opcode_lists() {
      this->raw.serialized = false;
   }
   void Trigger::generate_flat_opcode_lists(GameVariantDataMultiplayer& mp, std::vector<Condition*>& allConditions, std::vector<Action*>& allActions) {
      //
      // Opcodes are written to the file as flat lists: all conditions in the entire script, consecutively; then 
      // all actions in the entire script, consecutively; and then all triggers. Indices and counts in the triggers 
      // and conditions make it possible to reconstruct the triggers (and properly interleave conditions and actions) 
      // once all data is loaded.
      //
      // But... how is the data written?
      //
      // I examined some of Bungie's scripts by hand. It looks like for any given trigger:
      //
      //  - The opcodes of all nested triggers are written first.
      //  - Then, the trigger's own opcodes are written.
      //  - If a trigger doesn't contain any of a given kind of opcode, it'll still have a non-zero start index.
      //
      // As an example:
      //
      //    do              | Trigger 0 |           | Trigger indices are in order from outermost to innermost...
      //       condition    |           | Cond.n  2 | ...but opcodes are ordered from innermost to outermost.
      //       action       |           | Action  3 | 
      //       action       |           | Action  4 | 
      //       condition    |           | Cond.n  5 | 
      //       do           | Trigger 1 |           | 
      //          action    | Action  0 |           | 
      //          condition | Cond.n  0 |           | 
      //          action    | Action  1 |           | All content is sequential otherwise.
      //       end          |           |           | 
      //       do           | Trigger 2 |           | 
      //          action    | Action  2 |           | 
      //       end          |           |           | 
      //       action       |           | Action  5 | 
      //    end
      //
      // Below is Trigger 2 from Alpha Zombies, but we'll start our numbering off from 0 for simplicity. This time, 
      // we'll also note that nested triggers are "execute" actions.
      /*
            CODE               | TRIGGERS  | OPS    | BY SEQUENCE            |
            -------------------+-----------+--------+------------------------+
            do                 | Trigger 0 |        |    :    :    :    :    |
               do              | Trigger 1 | A |  9 |    |    |    |    |  9 |
                  condition    |           | C |  2 |    |  2 |    |    |    |
                  do           | Trigger 2 | A |  2 |    |  2 |    |    |    |
                     condition |           | C |  0 |  0 |    |    |    |    |
                     action    |           | A |  0 |  0 |    |    |    |    |
                  end          |           |        |    :    :    :    :    |
                  do           | Trigger 3 | A |  3 |    |  3 |    |    |    |
                     condition |           | C |  1 |  1 |    |    |    |    |
                     action    |           | A |  1 |  1 |    |    |    |    |
                  end          |           |        |    :    :    :    :    |
               end             |           |        |    :    :    :    :    |
               do              | Trigger 4 | A | 10 |    |    |    |    | 10 |
                  condition    |           | C |  5 |    |    |    |  5 |    |
                  action       |           | A |  6 |    |    |    |  6 |    |
                  do           | Trigger 5 | A |  7 |    |    |    |  7 |    |
                     condition |           | C |  3 |    |    |  3 |    |    |
                     action    |           | A |  4 |    |    |  4 |    |    |
                  end          |           |        |    :    :    :    :    |
                  do           | Trigger 6 | A |  8 |    |    |    |  8 |    |
                     condition |           | C |  4 |    |    |  4 |    |    |
                     action    |           | A |  5 |    |    |  5 |    |    |
                  end          |           |        |    :    :    :    :    |
               end             |           |        |    :    :    :    :    |
            end                |           |        |    :    :    :    :    |
      */
      // As you can see, triggers are numbered in sequence with the outermost first, while opcodes are numbered 
      // in sequence with the innermost first. However, this doesn't "cross boundaries." Triggers 2, 3, 5, and 6 
      // are all nested to the same depth, but Triggers 2 and 3 share a parent (Trigger 1) while Triggers 5 and 
      // 6 share a different parent (Trigger 4); therefore, Bungie's code serializes the opcodes for the triggers 
      // in this order: 2, 3, 1, 5, 6, 4; before then writing the opcodes for the root trigger 0.
      //
      if (this->raw.serialized) { // guard needed since we're doing nested triggers out of flat order
         return;
      }
      this->raw.serialized = true;
      //
      auto& triggers = mp.scriptContent.triggers;
      size_t size = this->opcodes.size();
      //
      // Recurse over nested triggers first, so that opcodes are serialized inner-first.
      //
      for (size_t i = 0; i < size; i++) {
         auto* opcode = this->opcodes[i];
         auto  action = dynamic_cast<const Action*>(opcode);
         if (action) {
            if (action->function == &actionFunction_runNestedTrigger) {
               auto arg = dynamic_cast<OpcodeArgValueTrigger*>(action->arguments[0]);
               assert(arg && "Found a Run Nested Trigger action with no argument specifying the trigger?!");
               auto i   = arg->value;
               assert(i >= 0 && i < triggers.size() && "Found a Run Nested Trigger action with an out-of-bounds index.");
               triggers[i].generate_flat_opcode_lists(mp, allConditions, allActions);
            }
         }
      }
      //
      // Now run over our own opcodes.
      //
      this->raw.conditionStart = allConditions.size();
      this->raw.conditionCount = 0;
      this->raw.actionStart    = allActions.size();
      this->raw.actionCount    = 0;
      for (size_t i = 0; i < size; ++i) {
         auto* opcode    = this->opcodes[i];
         auto  condition = dynamic_cast<Condition*>(opcode);
         if (condition) {
            this->raw.conditionCount += 1;
            allConditions.push_back(condition);
            //
            condition->action = this->raw.actionCount;
            //
            continue;
         }
         auto action = dynamic_cast<Action*>(opcode);
         if (action) {
            this->raw.actionCount += 1;
            allActions.push_back(action);
            continue;
         }
      }
   }
   //
   void Trigger::to_string(const std::vector<Trigger*>& allTriggers, std::string& out, std::string& indent) const noexcept {
      std::string line;
      //
      out += indent;
      out += "Block type: ";
      switch (this->blockType) {
         case block_type::normal:
            out += "normal";
            break;
         case block_type::for_each_object:
            out += "for each object";
            break;
         case block_type::for_each_object_with_label:
            if (!this->forgeLabel) {
               line = "for each object with label none";
            } else {
               ReachForgeLabel* f = this->forgeLabel;
               if (!f->name) {
                  cobb::sprintf(line, "label index %u", f->index);
                  break;
               }
               cobb::sprintf(line, "for each object with label %s", f->name->english().c_str());
            }
            out += line;
            break;
         case block_type::for_each_player:
            out += "for each player";
            break;
         case block_type::for_each_player_random:
            out += "for each player random?";
            break;
         case block_type::for_each_team:
            out += "for each team";
            break;
         default:
            cobb::sprintf(line, "unknown %u", (uint32_t)this->blockType);
      }
      out += "\r\n";
      //
      out += indent;
      out += "Entry type: ";
      switch (this->entryType) {
         case entry_type::local:             out += "local";          break;
         case entry_type::normal:            out += "normal";         break;
         case entry_type::on_host_migration: out += "host migration"; break;
         case entry_type::on_init:           out += "init";           break;
         case entry_type::on_local_init:     out += "local init";     break;
         case entry_type::on_object_death:   out += "object death";   break;
         case entry_type::pregame:           out += "pregame";        break;
         case entry_type::subroutine:        out += "subroutine";     break;
         default:
            cobb::sprintf(line, "unknown %u", (uint32_t)this->entryType);
      }
      out += "\r\n";
      //
      if (!this->opcodes.size()) {
         out += indent;
         out += "<Empty Trigger>";
      }
      int32_t last_condition_or_group = -1;
      for (auto& opcode : this->opcodes) {
         auto condition = dynamic_cast<const Condition*>(opcode);
         if (condition) {
            if (condition->or_group == last_condition_or_group) {
               cobb::sprintf(line, "%s[ OR] ", indent.c_str());
            } else {
               cobb::sprintf(line, "%s[CND] ", indent.c_str());
            }
            last_condition_or_group = condition->or_group;
            out += line;
            opcode->to_string(line);
            out += line;
            out += "\r\n";
            continue;
         }
         auto action = dynamic_cast<const Action*>(opcode);
         if (action) {
            if (action->function == &actionFunction_runNestedTrigger) {
               cobb::sprintf(line, "%s[ACT] Run nested trigger:\r\n", indent.c_str());
               out += line;
               //
               auto index = dynamic_cast<OpcodeArgValueTrigger*>(action->arguments[0]);
               if (index) {
                  auto i = index->value;
                  if (i < 0 || i >= allTriggers.size()) {
                     cobb::sprintf(line, "%s   <INVALID TRIGGER INDEX %d>\r\n", indent.c_str(), i);
                     out += line;
                     continue;
                  }
                  indent += "   ";
                  line.clear();
                  allTriggers[i]->to_string(allTriggers, line, indent);
                  out += line;
                  indent.resize(indent.size() - 3);
                  continue;
               }
               out += "   <INVALID>\r\n";
               continue;
            }
            cobb::sprintf(line, "%s[ACT] ", indent.c_str());
            out += line;
            opcode->to_string(line);
            out += line;
            out += "\r\n";
            continue;
         }
         out += indent;
         out += "Opcode with unrecognized type!\r\n";
      }
   }
   void Trigger::decompile(Decompiler& out) noexcept {
      //
      // We need to handle block structure, line breaks, and similar formatting here. Normally this would be 
      // pretty straightforward: each trigger is one single code block; we open another code block only when 
      // we find the opcode to run a nested trigger, right? Well yes, but actually no. Conditions can appear 
      // in the middle of a trigger and should be written as the start of a new if-block, which should wrap 
      // everything after those conditions. As such, a single trigger can actually be multiple blocks.
      //
      // If the trigger itself is an if-block (i.e. it's not a loop, and its first opcode is a condition), 
      // then we have to "open" the block not at the start of the trigger but rather when we start iterating 
      // over opcodes. We need a few bools to help keep track of this -- to know when to insert line breaks 
      // before "if" statements and so on.
      //
      auto mp = out.get_variant().get_multiplayer_data();
      if (!mp)
         return;
      auto& triggers = mp->scriptContent.triggers;
      //
      uint16_t indent_count          = 0;     // needed because if-blocks aren't "real;" we "open" one every time we encounter one or more conditions in a row, so we need to remember how many "end"s to write
      bool     is_first_opcode       = true;  // see comment for (trigger_is_if_block)
      bool     is_first_condition    = true;  // if we encounter a condition and this is (true), then we need to write "if "
      bool     trigger_is_if_block   = false; // we write an initial line break on each trigger; if this trigger is an if-block, then we need to avoid writing another line break when we actually write "if". this variable and (is_first_opcode) are used to facilitate this.
      bool     writing_if_conditions = false; // if we encounter an action and this is (true), then we need to reset (is_first_condition
      //
      // Write the initial line break, and write the event handler if any:
      //
      switch (this->entryType) { // TODO: use the list of event handler trigger indices instead of the individual triggers' entry types; if a trigger has an event entry type but isn't registered as an event handler, note that with a comment
         case entry_type::normal:
         case entry_type::subroutine:
            out.write_line("");
            break;
         case entry_type::local:
            out.write_line(u8"on local: "); // TODO: don't put this on its own line
            break;
         case entry_type::on_host_migration:
            out.write_line(u8"on host migration: ");
            break;
         case entry_type::on_init:
            out.write_line(u8"on init: ");
            break;
         case entry_type::on_local_init:
            out.write_line(u8"on local init: ");
            break;
         case entry_type::on_object_death:
            out.write_line(u8"on object death: ");
            break;
         case entry_type::pregame:
            out.write_line(u8"on pregame: ");
            break;
      }
      //
      // Write the block type, unless it's an if-block (do...end block whose first opcode is a condition), 
      // in which case we write the block type when we hit that first opcode.
      //
      switch (this->blockType) {
         case block_type::normal:
            {
               if (this->opcodes.size()) {
                  auto* first = this->opcodes[0];
                  auto* cnd   = dynamic_cast<Condition*>(first);
                  if (cnd) {
                     trigger_is_if_block = true;
                  } else {
                     out.write(u8"do");
                     out.modify_indent_count(1);
                     ++indent_count;
                  }
               } else {
                  out.write(u8"do");
                  out.modify_indent_count(1);
                  ++indent_count;
               }
            }
            break;
         case block_type::for_each_object:
            out.write("for each object do");
            out.modify_indent_count(1);
            ++indent_count;
            break;
         case block_type::for_each_object_with_label:
            {
               std::string line;
               if (!this->forgeLabel) {
                  line = "label none";
               } else {
                  ReachForgeLabel* f = this->forgeLabel;
                  if (!f->name)
                     cobb::sprintf(line, "label %u", f->index);
                  else
                     cobb::sprintf(line, "label \"%s\"", f->name->english().c_str()); // TODO: this will break if a label actually contains a double-quote
               }
               cobb::sprintf(line, "for each object with %s do", line.c_str());
               out.write(line);
               out.modify_indent_count(1);
               ++indent_count;
            }
            break;
         case block_type::for_each_player:
            out.write(u8"for each player do");
            out.modify_indent_count(1);
            ++indent_count;
            break;
         case block_type::for_each_player_random:
            out.write(u8"for each player randomly do");
            out.modify_indent_count(1);
            ++indent_count;
            break;
         case block_type::for_each_team:
            out.write(u8"for each team do");
            out.modify_indent_count(1);
            ++indent_count;
            break;
         default:
            out.write(u8"do -- unknown block type");
            out.modify_indent_count(1);
            ++indent_count;
            break;
      }
      //
      // Write the opcodes:
      //
      int32_t     last_condition_or_group = -1;
      std::string line;
      for (auto& opcode : this->opcodes) {
         auto condition = dynamic_cast<const Condition*>(opcode);
         bool _first    = is_first_opcode;
         is_first_opcode = false;
         if (condition) {
            if (is_first_condition) {
               if (!trigger_is_if_block || !_first) // if this trigger IS an if-block and we're writing the entire block's "if", then don't write a line break because we did that above
                  out.write_line("");
               out.write("if ");
               out.modify_indent_count(1);
               ++indent_count;
               is_first_condition    = false;
               writing_if_conditions = true;
            } else {
               if (condition->or_group == last_condition_or_group)
                  out.write(u8"or ");
               else
                  out.write(u8"and ");
            }
            last_condition_or_group = condition->or_group;
            opcode->decompile(out);
            out.write(' ');
            continue;
         }
         if (writing_if_conditions) {
            out.write(u8"then ");
            writing_if_conditions = false;
            is_first_condition    = true;
         }
         auto action = dynamic_cast<const Action*>(opcode);
         if (action) {
            if (action->function == &actionFunction_runNestedTrigger) {
               auto index = dynamic_cast<OpcodeArgValueTrigger*>(action->arguments[0]);
               if (index) {
                  auto i = index->value;
                  if (i < 0 || i >= triggers.size()) {
                     cobb::sprintf(line, u8"-- execute nested trigger with invalid index %d", i);
                     out.write_line(line);
                     continue;
                  }
                  triggers[i].decompile(out);
                  continue;
               }
               out.write_line(u8"-- invalid \"execute nested trigger\" opcode");
               continue;
            }
            out.write_line("");
            opcode->decompile(out);
            continue;
         }
         out.write_line(u8"-- invalid opcode type");
      }
      //
      // Close all open blocks. Remember: conditions encountered in the middle of the trigger count 
      // as new if-blocks that we have to open, so we have to de-indent and write an "end" keyword 
      // for each of those as well as for the trigger itself.
      //
      while (indent_count-- > 0) {
         out.modify_indent_count(-1);
         out.write_line(u8"end");
      }
   }
}
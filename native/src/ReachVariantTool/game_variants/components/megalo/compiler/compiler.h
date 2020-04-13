#pragma once
#include <functional>
#include <vector>
#include <QString>
#include "string_scanner.h"
#include "../trigger.h"
#include "parsing/base.h"
#include "parsing/alias.h"
#include "parsing/variable_reference.h"

namespace Megalo {
   class Compiler;
   //
   namespace Script {
      class Block : public ParsedItem {
         public:
            enum class Type : uint8_t {
               basic, // do
               root,
               if_block,
               elseif_block,
               else_block,
               for_each_object,
               for_each_object_with_label,
               for_each_player,
               for_each_player_randomly,
               for_each_team,
               function,
            };
            enum class Event : uint8_t {
               none,
               init,
               local_init,
               host_migration,
               double_host_migration,
               object_death,
               local,
               pregame,
            };
            //
         public:
            Trigger* trigger  = nullptr;
            QString  name; // only for functions
            QString  label_name;
            int32_t  label_index = -1;
            Type     type;
            Event    event;
            std::vector<ParsedItem*> items; // contents are owned by this Block and deleted in the destructor
            std::vector<ParsedItem*> conditions; // only for if/elseif blocks // contents are owned by this Block and deleted in the destructor
            //
            ~Block();
            void insert_condition(ParsedItem*);
            void insert_item(ParsedItem*);
            ParsedItem* item(int32_t); // negative indices wrap around, i.e. -1 is the last element
      };
      class Statement : public ParsedItem {
         public:
            Opcode* opcode = nullptr; // a fully-compiled opcode
            VariableReference* lhs = nullptr; // owned by this Statement and deleted in the destructor
            VariableReference* rhs = nullptr; // owned by this Statement and deleted in the destructor
            QString op;
            //
            ~Statement();
      };
      class Comparison : public Statement {
         public:
            bool negated = false; // TODO: not needed if we compile the Opcode when it's found; see: Compiler::negate_next_condition
      };
      class UserDefinedFunctionCall : public ParsedItem {
         public:
            Block* target = nullptr;
      };
   }
   //
   class Compiler : public Script::string_scanner {
      public:
         struct Token {
            QString text;
            Script::string_scanner::pos pos;
            bool ended = false; // whether we hit whitspace, meaning that the current word is "over"
            bool brace = false; // whether we're in square brackets; needed to properly handle constructs like "abc[ d]" or "abc[-1]" at the starts of statements
         };
         using scan_functor_t = std::function<bool(QChar)>;
         //
      protected:
         Script::Block*      root       = nullptr;
         Script::Block*      block      = nullptr; // current block being parsed
         Script::Statement*  assignment = nullptr; // current assignment being parsed, if any
         Script::Comparison* comparison = nullptr; // current comparison being parsed, if any
         Token token;
         Script::Block::Event next_event = Script::Block::Event::none;
         bool negate_next_condition = false;
         std::vector<Script::Alias*> aliases_in_scope;
         std::vector<Script::Block*> functions_in_scope;
         //
      public:
         void throw_error(const QString& text);
         void throw_error(const Script::string_scanner::pos& pos, const QString& text);
         void reset_token();
         //
         void parse(QString text); // can throw compile_exception
         //
         static bool is_keyword(QString word);
         //
         Script::Alias* lookup_relative_alias(QString name, const OpcodeArgTypeinfo* relative_to);
         Script::Alias* lookup_absolute_alias(QString name);
         Script::Block* lookup_user_defined_function(QString name);
         //
         enum class name_source {
            none,
            action,
            condition,
            imported_name,
            namespace_member,
            static_typename,
            variable_typename,
         };
         static name_source check_name_is_taken(const QString& name, OpcodeArgTypeRegistry::type_list_t& name_is_imported_from);
         //
      protected:
         bool is_in_statement() const;
         //
         void _parseActionStart(QChar);
         void _parseAssignment(QChar);
         //
         void _parseBlockConditions();
         bool _parseConditionStart(QChar); // returns "true" at the end of the condition list, i.e. upon reaching the keywrod "then"
         void _parseComparison(QChar);
         //
         void _applyConditionModifiers(Condition&); // applies "not", "and", "or", and then resets the relevant state on the Compiler
         //
         void __parseFunctionArgs(const OpcodeBase&, Opcode&);
         void _parseFunctionCall(bool is_condition);
         //
         bool _closeCurrentBlock();
         //
         extract_result_t extract_integer_literal(int32_t& out) {
            auto result = string_scanner::extract_integer_literal(out);
            if (result == string_scanner::extract_result::floating_point)
               this->throw_error("Unexpected decimal point. Floating-point numbers are not supported.");
            return result;
         }
         //
         void _handleKeyword_Alias();
         void _handleKeyword_Declare(); // INCOMPLETE
         void _handleKeyword_Do();
         void _handleKeyword_Else();
         void _handleKeyword_ElseIf();
         void _handleKeyword_End();
         void _handleKeyword_If();
         void _handleKeyword_For();
         void _handleKeyword_Function();
         void _handleKeyword_On();
   };
}
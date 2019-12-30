#pragma once
#include <cstdint>
#include <string>
#include <type_traits>
#include "../../../helpers/bitwise.h"

namespace Megalo {
   class SmartEnum {
      public:
         constexpr SmartEnum(const char** v, uint32_t n, bool o = false) : values(v), count(n), offset(o) {}
         //
         const char** values = nullptr;
         uint32_t     count  = 0;
         bool         offset = false;
         //
         constexpr inline int count_bits() const noexcept {
            if (!this->count)
               return 0;
            return cobb::bitcount(this->count);
         }
         constexpr inline int index_bits() const noexcept {
            if (!this->count)
               return 0;
            return cobb::bitcount(this->count + (this->offset ? 1 : 0) - 1);
         }
         constexpr inline int index_bits_with_offset(uint32_t offset) const noexcept {
            if (!this->count)
               return 0;
            return cobb::bitcount(this->count + offset - 1);
         }
         //
         constexpr bool is_in_bounds(int v) const noexcept {
            if (v < 0) {
               if (this->offset)
                  return v == -1;
               return false;
            }
            return v < this->count;
         }
         //
         void to_string(std::string& out, uint32_t value) const noexcept;
         //
         const char* operator[](size_t a) const noexcept {
            if (this->offset)
               ++a;
            if (a < this->count)
               return this->values[a];
            return nullptr;
         }
         //
         int32_t lookup(const char* value) const noexcept {
            for (uint32_t i = 0; i < this->count; i++)
               if (_stricmp(value, this->values[i]) == 0)
                  return i - (this->offset ? 1 : 0);
            return -1 - (this->offset ? 1 : 0);
         }
         //
         SmartEnum* slice(size_t from, size_t to) const noexcept {
            size_t size = to - from;
            const char** pointers = (const char**)malloc(sizeof(const char*) * size);
            for (size_t i = 0; i < size; i++)
               #pragma warning(suppress: 6011) // if we didn't have enough memory, then malloc returned nullptr, which we are deferencing... but not having memory is a larger and irrecoverable problem anyway
               pointers[i] = this->values[i + to];
            return new SmartEnum(pointers, size);
         }
   };
   #define megalo_define_smart_enum(name, ...) namespace { namespace __smart_enum_members { const char* _##name##[] = { __VA_ARGS__ }; } }; extern const ::Megalo::SmartEnum name = ::Megalo::SmartEnum(__smart_enum_members::_##name##, std::extent<decltype(__smart_enum_members::_##name##)>::value);

   class SmartFlags {
      public:
         constexpr SmartFlags(const char** v, uint32_t n) : values(v), count(n) {}
         //
         const char** values = nullptr;
         uint32_t     count  = 0;
         //
         constexpr inline int bits() const noexcept {
            return this->count;
         }
         //
         void to_string(std::string& out, uint32_t value) const noexcept;
   };
   #define megalo_define_smart_flags(name, ...) namespace { namespace __smart_flags_members { const char* _##name##[] = { __VA_ARGS__ }; } }; extern const ::Megalo::SmartFlags name = ::Megalo::SmartFlags(__smart_flags_members::_##name##, std::extent<decltype(__smart_flags_members::_##name##)>::value);
}
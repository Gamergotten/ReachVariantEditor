#include "localized_string_table.h"
extern "C" {
   #include "../../zlib/zlib.h" // interproject ref
}

void ReachString::read_offsets(cobb::bitstream& stream, ReachStringTable& table) noexcept {
   for (size_t i = 0; i < this->offsets.size(); i++) {
      bool has = stream.read_bits(1) != 0;
      if (has)
         this->offsets[i] = stream.read_bits(table.offset_bitlength);
      else
         this->offsets[i] = -1;
   }
}
void ReachString::read_strings(void* buffer) noexcept {
   for (size_t i = 0; i < this->offsets.size(); i++) {
      auto off = this->offsets[i];
      if (off == -1)
         continue;
      this->strings[i].assign((const char*)((std::uintptr_t)buffer + off));
   }
}

void* ReachStringTable::_make_buffer(cobb::bitstream& stream) const noexcept {
   uint32_t uncompressed_size = stream.read_bits(this->buffer_size_bitlength);
   bool     is_compressed     = stream.read_bits(1) != 0;
   uint32_t size = uncompressed_size;
   if (is_compressed)
      size = stream.read_bits(this->buffer_size_bitlength);
   //
   void* buffer = malloc(size);
   for (uint32_t i = 0; i < size; i++)
      *(uint8_t*)((std::uintptr_t)buffer + i) = stream.read_bits<uint8_t>(8);
   if (is_compressed) {
      //
      // The buffer has four bytes indicating the length of the decompressed data, which is 
      // redundant. We'll read those four bytes just as a sanity check, but we need to skip 
      // them -- zlib needs to receive (buffer + 4) as its input.
      //
      uint32_t uncompressed_size_2 = *(uint32_t*)(buffer);
      if (uncompressed_size_2 != uncompressed_size)
         if (_byteswap_ulong(uncompressed_size_2) != uncompressed_size)
            printf("WARNING: String table sizes don't match: Bungie 0x%08X versus zlib 0x%08X.", uncompressed_size, uncompressed_size_2);
      void* final = malloc(uncompressed_size);
      //
      // It's normally pointless to check that an allocation succeeded because if it didn't, 
      // that means you're out of memory and there's basically nothing you can do about that 
      // anyway. However, if we screwed up and misread the size, then we may try to allocate 
      // a stupidly huge amount of memory. In that case, allocation will (hopefully) fail and 
      // (final) will be nullptr, and that's worth handling.
      //
      if (final) {
         Bytef* input = (Bytef*)((std::uintptr_t)buffer + sizeof(uncompressed_size_2));
         int resultCode = uncompress((Bytef*)final, (uLongf*)&uncompressed_size_2, input, size - sizeof(uncompressed_size_2));
         if (Z_OK != resultCode) {
            free(final);
            final = nullptr;
            printf("Failed to decompress zlib stream. Failure code was %d.\n", resultCode);
         }
      }
      free(buffer);
      buffer = final;
   }
   return buffer;
}
void ReachStringTable::read(cobb::bitstream& stream) noexcept {
   size_t count = stream.read_bits(this->count_bitlength);
   this->strings.resize(count);
   for (int i = 0; i < count; i++)
      this->strings[i].read_offsets(stream, *this);
   if (count) {
      auto buffer = this->_make_buffer(stream);
      if (buffer) {
         for (size_t i = 0; i < count; i++)
            this->strings[i].read_strings(buffer);
         free(buffer);
      }
   }
}
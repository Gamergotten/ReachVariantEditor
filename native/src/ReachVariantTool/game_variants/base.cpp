#include "base.h"
#include "../helpers/bitreader.h"
#include "../helpers/bitwriter.h"

#include "../formats/sha1.h"
#include "../helpers/sha1.h"

#include "types/multiplayer.h"
#include "errors.h"

bool BlamHeader::read(cobb::bytereader& stream) noexcept {
   if (!this->header.read(stream))
      return false;
   stream.read(this->data.unk0C, cobb::endian::big); // endianness assumed but not known
   stream.read(this->data.unk0E, cobb::endian::big); // endianness assumed but not known
   stream.read(this->data.unk2E, cobb::endian::big); // endianness assumed but not known
   return true;
}
void BlamHeader::write(cobb::bytewriter& stream) const noexcept {
   this->header.write(stream);
   stream.enlarge_by(0x24);
   stream.write(this->data.unk0C, cobb::endian::big);
   stream.write(this->data.unk0E);
   stream.write(this->data.unk2E);
   this->header.write_postprocess(stream);
}

bool EOFBlock::read(cobb::bytereader& stream) noexcept {
   if (!ReachFileBlock::read(stream))
      return false;
   stream.read(this->length);
   stream.read(this->unk04);
   return true;
}
void EOFBlock::write(cobb::bytewriter& stream) const noexcept {
   uint32_t myPos = stream.get_bytespan();
   ReachFileBlock::write(stream);
   stream.write(myPos, cobb::endian::big); // NOTE: in some files this is little-endian, without the block having a different version or flags
   stream.write(uint8_t(0));
}

bool GameVariantHeader::read(cobb::bitreader& stream) noexcept {
   this->build.major = 0; // not in mpvr
   this->build.minor = 0; // not in mpvr
   this->contentType.read(stream);
   this->fileLength.read(stream);
   this->unk08.read(stream);
   this->unk10.read(stream);
   this->unk18.read(stream);
   this->unk20.read(stream);
   this->activity.read(stream);
   this->gameMode.read(stream);
   this->engine.read(stream);
   this->unk2C.read(stream);
   this->engineCategory.read(stream);
   this->createdBy.read(stream);
   this->modifiedBy.read(stream);
   stream.read_u16string(this->title, 128); // big-endian
   this->title[127] = '\0';
   stream.read_u16string(this->description, 128); // big-endian
   this->description[127] = '\0';
   if (this->contentType == ReachFileType::game_variant) {
      this->engineIcon.read(stream);
   }
   if (this->activity == 2)
      stream.skip(16); // TODO: hopper ID
   return true;
}
bool GameVariantHeader::read(cobb::bytereader& stream) noexcept {
   this->build.major.read(stream);
   this->build.minor.read(stream);
   this->contentType.read(stream);
   stream.pad(3);
   this->fileLength.read(stream);
   this->unk08.read(stream);
   this->unk10.read(stream);
   this->unk18.read(stream);
   this->unk20.read(stream);
   this->activity.read(stream);
   this->gameMode.read(stream);
   this->engine.read(stream);
   stream.pad(1);
   this->unk2C.read(stream);
   this->engineCategory.read(stream);
   stream.pad(4);
   this->createdBy.read(stream);
   this->modifiedBy.read(stream);
   stream.read_u16string(this->title, 128);
   this->title[127] = '\0';
   stream.read_u16string(this->description, 128);
   this->description[127] = '\0';
   this->engineIcon.read(stream);
   stream.read(this->unk284);
   return true;
}
void GameVariantHeader::write(cobb::bitwriter& stream) const noexcept {
   this->contentType.write(stream);
   this->writeData.offset_of_file_length = stream.get_bitpos();
   this->fileLength.write(stream); // handled fully in the write_last_minute_fixup member function
   this->unk08.write(stream);
   this->unk10.write(stream);
   this->unk18.write(stream);
   this->unk20.write(stream);
   this->activity.write(stream);
   this->gameMode.write(stream);
   this->engine.write(stream);
   this->unk2C.write(stream);
   this->engineCategory.write(stream);
   this->createdBy.write(stream);
   this->modifiedBy.write(stream);
   stream.write_u16string(this->title,       128); // big-endian
   stream.write_u16string(this->description, 128); // big-endian
   if (this->contentType == ReachFileType::game_variant) {
      this->engineIcon.write(stream);
   }
   if (this->activity == 2)
      assert(false && "Hopper ID writing not implemented!"); // TODO: hopper ID
}
void GameVariantHeader::write(cobb::bytewriter& stream) const noexcept {
   this->build.major.write(stream);
   this->build.minor.write(stream);
   this->contentType.write(stream);
   stream.pad(3);
   this->writeData.offset_of_file_length = stream.get_bytepos();
   this->fileLength.write(stream); // handled fully in the write_last_minute_fixup member function
   this->unk08.write(stream);
   this->unk10.write(stream);
   this->unk18.write(stream);
   this->unk20.write(stream);
   this->activity.write(stream);
   this->gameMode.write(stream);
   this->engine.write(stream);
   stream.pad(1);
   this->unk2C.write(stream);
   this->engineCategory.write(stream);
   stream.pad(4);
   this->createdBy.write(stream);
   this->modifiedBy.write(stream);
   stream.write(this->title);
   stream.write(this->description);
   this->engineIcon.write(stream);
   stream.write(this->unk284);
}
void GameVariantHeader::write_last_minute_fixup(cobb::bitwriter& stream) const noexcept {
   stream.write_to_bitpos(this->writeData.offset_of_file_length, cobb::bits_in<uint32_t>, stream.get_bytespan());
}
void GameVariantHeader::write_last_minute_fixup(cobb::bytewriter& stream) const noexcept {
   stream.write_to_offset(this->writeData.offset_of_file_length, stream.get_bytespan(), cobb::endian::little);
}
//
void GameVariantHeader::set_title(const char16_t* value) noexcept {
   memset(this->title, 0, sizeof(this->title));
   for (size_t i = 0; i < std::extent<decltype(this->title)>::value; i++) {
      char16_t c = value[i];
      if (!c)
         break;
      this->title[i] = c;
   }
}
void GameVariantHeader::set_description(const char16_t* value) noexcept {
   memset(this->description, 0, sizeof(this->description));
   for (size_t i = 0; i < std::extent<decltype(this->description)>::value; i++) {
      char16_t c = value[i];
      if (!c)
         break;
      this->description[i] = c;
   }
}

bool ReachBlockMPVR::read(cobb::bit_or_byte_reader& reader) {
   auto& error_report = GameEngineVariantLoadError::get();
   //
   uint32_t offset_before_hashable;
   uint32_t offset_after_hashable;
   //
   if (!this->header.read(reader.bytes)) {
      return false;
   }
   reader.synchronize();
   auto& stream = reader.bits;
   //
   stream.read(this->hashSHA1);
   stream.skip(4 * 8); // skip four unused bytes
   stream.skip(4 * 8); // == size of variant data in big-endian, i.e. offset_after_hashable - offset_before_hashable
   offset_before_hashable = stream.get_bytespan();
   this->type.read(stream);
   switch (this->type) {
      case ReachGameEngine::multiplayer:
      case ReachGameEngine::forge:
         this->data = new GameVariantDataMultiplayer(this->type == ReachGameEngine::forge);
         break;
      case ReachGameEngine::none: // TODO: Ask the user what type we should use.
         // fall through
      case ReachGameEngine::campaign:
         // fall through
      case ReachGameEngine::firefight:
         error_report.state         = GameEngineVariantLoadError::load_state::failure;
         error_report.failure_point = GameEngineVariantLoadError::load_failure_point::variant_type;
         error_report.extra[0]      = (int32_t)this->type;
         return false;
      default:
         printf("Variant has unknown type. Can't load it.\n"); // TODO: Ask the user what type we should use.
         return false;
   }
   if (!this->data->read(reader)) {
      error_report.state = GameEngineVariantLoadError::load_state::failure;
      return false;
   }
   offset_after_hashable = stream.get_bytespan();
   if (reader.overshot_eof()) {
      error_report.state  = GameEngineVariantLoadError::load_state::failure;
      error_report.reason = GameEngineVariantLoadError::load_failure_reason::early_eof;
      return false;
   }
   this->remainingData.read(stream, this->header.end()); // TODO: this can fail and it'll signal errors to (error_report) appropriately; should we even care?
      //
      // Specifically, a 360-era modded gametype, "SvE Mythic Infection," ends its MPVR block early but still has a full-size block length i.e. 0x5028, so 
      // the _eof block ends up inside of here AND we hit the actual end-of-file early, causing (remainingData.read) to fail. However, we can still read 
      // the game variant data; I haven't tested whether MCC can.
   //
   {
      auto     hasher   = cobb::sha1();
      uint32_t size     = offset_after_hashable - offset_before_hashable;
      printf("Checking SHA-1 hash... Data size is %08X (%08X - %08X).\n", size, offset_before_hashable, offset_after_hashable);
      uint32_t bufsize  = size + sizeof(reachSHA1Salt) + 4;
      auto     buffer   = (const uint8_t*)stream.data();
      auto     buffer32 = (const uint32_t*)buffer;
      uint8_t* working  = (uint8_t*)malloc(bufsize);
      memcpy(working, reachSHA1Salt, sizeof(reachSHA1Salt));
      *(uint32_t*)(working + sizeof(reachSHA1Salt)) = cobb::to_big_endian(uint32_t(size));
      memcpy(working + bufsize - size, buffer + offset_before_hashable, size);
      hasher.transform(working, bufsize);
      free(working);
      working = nullptr;
      //
      printf("File's existing hash:\n");
      for (int i = 0; i < 5; i++)
         printf("   %08X\n", buffer32[0x2FC / 4 + i]);
      printf("\nOur hash:\n");
      for (int i = 0; i < 5; i++)
         printf("   %08X\n", hasher.hash[i]);
      printf("Check done.\n");
   }
   //
   return true;
}
void ReachBlockMPVR::write(cobb::bit_or_byte_writer& writer) const noexcept {
   auto& bytes = writer.bytes;
   auto& bits  = writer.bits;
   auto& wd    = this->writeData;

   uint32_t offset_of_hash;
   uint32_t offset_before_hashable;
   uint32_t offset_after_hashable;
   uint32_t offset_of_hashable_length;
   this->header.write(bytes);
   wd.offset_of_hash = bytes.get_bytespan();
   bytes.write(this->hashSHA1);
   bytes.pad(4); // four unused bytes
   wd.offset_of_hashable_length = bytes.get_bytespan();
   bytes.pad(4); // handled in (write_last_minute_fixup)
   wd.offset_before_hashable = bytes.get_bytespan();
   //
   writer.synchronize();
   //
   if (this->data) {
      //
      // Use the data object's type if the value is sensible; we need to do this to allow 
      // switching variants between multiplayer and Forge.
      //
      auto t = this->data->get_type();
      if (t != ReachGameEngine::none) {
         decltype(this->type) dummy = t;
         dummy.write(bits);
      }
   } else
      this->type.write(bits);

   if (this->data)
      this->data->write(writer);

   wd.offset_after_hashable = bits.get_bytespan();
   //
   writer.synchronize();
   //
   bytes.pad_to_bytepos(this->header.write_end());
   this->header.write_postprocess(bytes);
   //
   writer.synchronize();
}
void ReachBlockMPVR::write_last_minute_fixup(cobb::bit_or_byte_writer& writer) const noexcept {
   auto& wd    = this->writeData;
   auto& bytes = writer.bytes;
   writer.synchronize();
   //
   if (this->data)
      this->data->write_last_minute_fixup(writer);
   {  // SHA-1 hash
      auto hasher = cobb::sha1();
      uint32_t size = wd.offset_after_hashable - wd.offset_before_hashable;
      bytes.write_to_offset(wd.offset_of_hashable_length, size, cobb::endian::little);
      uint32_t bufsize  = size + sizeof(reachSHA1Salt) + 4;
      auto     buffer   = (const uint8_t*)bytes.data();
      auto     buffer32 = (const uint32_t*)buffer;
      uint8_t* working  = (uint8_t*)malloc(bufsize);
      memcpy(working, reachSHA1Salt, sizeof(reachSHA1Salt));
      *(uint32_t*)(working + sizeof(reachSHA1Salt)) = cobb::to_big_endian(size);
      memcpy(working + bufsize - size, buffer + 0x318, size);
      hasher.transform(working, bufsize);
      free(working);
      //
      bytes.write_to_offset(wd.offset_of_hash + 0x00, hasher.hash[0], cobb::endian::big);
      bytes.write_to_offset(wd.offset_of_hash + 0x04, hasher.hash[1], cobb::endian::big);
      bytes.write_to_offset(wd.offset_of_hash + 0x08, hasher.hash[2], cobb::endian::big);
      bytes.write_to_offset(wd.offset_of_hash + 0x0C, hasher.hash[3], cobb::endian::big);
      bytes.write_to_offset(wd.offset_of_hash + 0x10, hasher.hash[4], cobb::endian::big);
   }
   //
   writer.synchronize();
}
void ReachBlockMPVR::cloneTo(ReachBlockMPVR& target) const noexcept {
   target = *this;
   target.data = this->data->clone();
   this->remainingData.cloneTo(target.remainingData);
}

bool GameVariant::read(cobb::mapped_file& file) {
   auto& error_report = GameEngineVariantLoadError::get();
   error_report.reset();
   //
   auto reader = cobb::bit_or_byte_reader(file.data(), file.size());
   if (!this->blamHeader.read(reader.bytes)) {
      error_report.state         = GameEngineVariantLoadError::load_state::failure;
      error_report.failure_point = GameEngineVariantLoadError::load_failure_point::block_blam;
      //
      auto& bh = this->blamHeader.header;
      if (bh.found.signature && bh.found.signature != bh.expected.signature) {
         error_report.reason = GameEngineVariantLoadError::load_failure_reason::not_a_blam_file;
      }
      return false;
   }
   if (!reader.is_in_bounds()) {
      error_report.state         = GameEngineVariantLoadError::load_state::failure;
      error_report.failure_point = GameEngineVariantLoadError::load_failure_point::block_blam;
      error_report.reason        = GameEngineVariantLoadError::load_failure_reason::early_eof;
      return false;
   }
   if (!this->contentHeader.read(reader.bytes)) {
      error_report.state         = GameEngineVariantLoadError::load_state::failure;
      error_report.failure_point = GameEngineVariantLoadError::load_failure_point::block_chdr;
      return false;
   }
   reader.synchronize();
   if (!reader.is_in_bounds()) {
      error_report.state         = GameEngineVariantLoadError::load_state::failure;
      error_report.failure_point = GameEngineVariantLoadError::load_failure_point::block_chdr;
      error_report.reason        = GameEngineVariantLoadError::load_failure_reason::early_eof;
      return false;
   }
   if (this->contentHeader.data.contentType != ReachFileType::game_variant) {
      error_report.state         = GameEngineVariantLoadError::load_state::failure;
      error_report.failure_point = GameEngineVariantLoadError::load_failure_point::content_type;
      error_report.extra[0]      = (int32_t)this->contentHeader.data.contentType;
      return false;
   }
   {  // mpvr (including as _cmp)
      bool result = false;
      uint32_t signature = 0;
      reader.bytes.peek(signature, cobb::endian::big);
      if (signature == '_cmp') {
         ReachFileBlockCompressed cblock;
         cblock.read(reader.bytes);
         //
         auto mpvr_reader = cobb::bit_or_byte_reader(cblock.buffer, cblock.size_inflated);
         result = this->multiplayer.read(mpvr_reader);
      } else {
         result = this->multiplayer.read(reader);
      }
      if (!result) {
         error_report.state = GameEngineVariantLoadError::load_state::failure;
         if (!error_report.has_failure_point())
            error_report.failure_point = GameEngineVariantLoadError::load_failure_point::block_mpvr;
         return false;
      }
      reader.synchronize();
   }
   //
   while (file.is_in_bounds(reader.bytes.get_bytepos(), 4)) {
      uint32_t signature = 0;
      reader.bytes.peek(signature, cobb::endian::big);
      if (signature == '_eof') {
         this->eofBlock.read(reader.bytes);
         break;
      } else {
         auto& block = this->unknownBlocks.emplace_back();
         if (!block.read(reader.bytes) || !block.header.found.signature) {
            this->unknownBlocks.resize(this->unknownBlocks.size() - 1);
            break;
         }
      }
   }
   return true;
}
void GameVariant::write(cobb::bit_or_byte_writer& writer) const noexcept {
   this->blamHeader.write(writer.bytes);
   this->contentHeader.write(writer.bytes);
   writer.synchronize();
   this->multiplayer.write(writer);
   for (auto& unknown : this->unknownBlocks)
      unknown.write(writer.bytes);
   this->eofBlock.write(writer.bytes);
   //
   this->contentHeader.write_last_minute_fixup(writer.bytes);
   this->multiplayer.write_last_minute_fixup(writer);
}
void GameVariant::test_mpvr_hash(cobb::mapped_file& file) noexcept {
   printf("Testing our hashing algorithm on this game variant...\n");
   //
   const uint8_t*  buffer   = (const uint8_t*)file.data();
   const uint32_t* buffer32 = (const uint32_t*)buffer;
   //
   uint32_t signature = cobb::from_big_endian(buffer32[0x2F0 / 4]);
   if (signature != 'mpvr') {
      printf("Failed to find the mpvr block; signature found was %08X when we expected %08X.\n", signature, 'mpvr');
      return;
   }
   //
   // Bungie only hashes the portion of MPVR that the game variant actually uses. This bytecount is a 
   // big-endian uint32_t located 0x4 bytes after the hash (i.e. offset 0x314 in the file). We need to 
   // read that, and then only run the hashing algorithm on that many bytes after that length value.
   //
   uint32_t size = cobb::from_big_endian(buffer32[0x314 / 4]);
   printf("File's existing hash (%04X bytes of data):\n", size);
   for (int i = 0; i < 5; i++)
      printf("   %08X\n", buffer32[0x2FC / 4 + i]);
   printf("\n");
   //
   auto hasher = cobb::sha1();
   {
      uint32_t bufsize  = size + sizeof(reachSHA1Salt) + 4;
      auto     buffer   = (const uint8_t*)file.data();
      auto     buffer32 = (const uint32_t*)buffer;
      uint8_t* working  = (uint8_t*)malloc(bufsize);
      memcpy(working, reachSHA1Salt, sizeof(reachSHA1Salt));
      *(uint32_t*)(working + sizeof(reachSHA1Salt)) = cobb::to_big_endian(size);
      memcpy(working + bufsize - size, buffer + 0x318, size);
      hasher.transform(working, bufsize);
      free(working);
   }
   printf("Our hash:\n");
   for (int i = 0; i < 5; i++)
      printf("   %08X\n", hasher.hash[i]);
   printf("Test done.\n");
}
GameVariantDataMultiplayer* GameVariant::get_multiplayer_data() const noexcept {
   auto d = this->multiplayer.data;
   if (!d)
      return nullptr;
   return d->as_multiplayer();
}

GameVariant* GameVariant::clone() const noexcept {
   GameVariant* clone = new GameVariant();
   clone->blamHeader    = this->blamHeader;
   clone->contentHeader = this->contentHeader;
   this->multiplayer.cloneTo(clone->multiplayer);
   clone->eofBlock      = this->eofBlock;
   clone->unknownBlocks = this->unknownBlocks;
   return clone;
}
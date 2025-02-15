#include "function_record.h"

#include "arch/common/instruction_relocator.h"
#include "arch/common/trampoline.h"
#include "logger.h"
#include "memory.h"

namespace rv64hook {

static constexpr const char* kTag = "Hook";

FunctionRecord::FunctionRecord(func_t address)
    : address_(address),
      backup_trampoline_(nullptr),
      original_function_hash_(0),
      overwrite_size_(0) {
}

bool FunctionRecord::IsModified() {
  return ComputeHash(address_, overwrite_size_) != original_function_hash_;
}

int FunctionRecord::WriteTrampoline(func_t hook, func_t* backup) {
  if (backup_trampoline_) {
    Memory::Free(backup_trampoline_);
  }

  auto type = Trampoline::GetSuggestedTrampolineType(address_, hook);
  overwrite_size_ = Trampoline::GetFirstTrampolineSize(type);

  if (!Memory::Copy(function_backup_, address_, overwrite_size_)) [[unlikely]] {
    SET_ERROR("Function is not readable");
    return 0;
  }

  original_function_hash_ = ComputeHash(address_, overwrite_size_);

  if (backup && !InstructionRelocator::Relocate(address_, overwrite_size_, backup)) [[unlikely]] {
    return 0;
  }
  backup_trampoline_ = *backup;

  if (!Trampoline::WriteFirstTrampoline(address_, hook, type)) [[unlikely]] {
    SET_ERROR("Function is not writable");
    return 0;
  }

  return overwrite_size_;
}

void FunctionRecord::Unhook() {
  Memory::Copy(address_, function_backup_, overwrite_size_);
  if (backup_trampoline_) {
    Memory::Free(backup_trampoline_);
  }
}

int16_t FunctionRecord::ComputeHash(func_t address, size_t size) {
  auto buf = reinterpret_cast<int8_t*>(address);
  int16_t hash = 17;
  for (auto limit = buf + size; buf < limit; ++buf) {
    hash = 31 * hash + *buf;
  }
  return hash;
}

}  // namespace rv64hook
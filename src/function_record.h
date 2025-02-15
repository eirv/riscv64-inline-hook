#pragma once

#include "arch/common/trampoline.h"
#include "rv64hook.h"

namespace rv64hook {

class FunctionRecord {
 public:
  FunctionRecord(func_t address);

  bool IsModified();

  bool WriteTrampoline(func_t hook, func_t* backup);

  void Unhook();

 private:
  func_t address_;
  void* backup_trampoline_;
  int16_t original_function_hash_;
  uint8_t function_backup_size_;
  uint8_t function_backup_[kMaxBackupSize];

  static int16_t ComputeHash(func_t address, size_t size);
};

}  // namespace rv64hook

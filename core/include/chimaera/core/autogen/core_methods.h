#ifndef WRPCTE_CORE_AUTOGEN_METHODS_H_
#define WRPCTE_CORE_AUTOGEN_METHODS_H_

#include <chimaera/chimaera.h>

namespace wrp_cte::core {

namespace Method {
  // Inherited methods (always include these)
  GLOBAL_CONST chi::u32 kCreate = 0;
  GLOBAL_CONST chi::u32 kDestroy = 1;
  GLOBAL_CONST chi::u32 kNodeFailure = 2;
  GLOBAL_CONST chi::u32 kRecover = 3;
  GLOBAL_CONST chi::u32 kMigrate = 4;
  GLOBAL_CONST chi::u32 kUpgrade = 5;
  
  // Module-specific methods (starting from 10)
  GLOBAL_CONST chi::u32 kRegisterTarget = 10;
  GLOBAL_CONST chi::u32 kUnregisterTarget = 11;
  GLOBAL_CONST chi::u32 kListTargets = 12;
  GLOBAL_CONST chi::u32 kStatTargets = 13;
  GLOBAL_CONST chi::u32 kGetOrCreateTag = 14;
  GLOBAL_CONST chi::u32 kPutBlob = 15;
  GLOBAL_CONST chi::u32 kGetBlob = 16;
  GLOBAL_CONST chi::u32 kReorganizeBlob = 17;
}

} // namespace wrp_cte::core

#endif // WRPCTE_CORE_AUTOGEN_METHODS_H_
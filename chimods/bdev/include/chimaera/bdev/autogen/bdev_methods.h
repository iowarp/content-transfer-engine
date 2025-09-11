#ifndef BDEV_AUTOGEN_METHODS_H_
#define BDEV_AUTOGEN_METHODS_H_

#include <chimaera/chimaera.h>

namespace chimaera::bdev_extended {

namespace Method {
  // Inherited methods (always include these)
  GLOBAL_CONST chi::u32 kCreate = 0;
  GLOBAL_CONST chi::u32 kDestroy = 1;
  GLOBAL_CONST chi::u32 kNodeFailure = 2;
  GLOBAL_CONST chi::u32 kRecover = 3;
  GLOBAL_CONST chi::u32 kMigrate = 4;
  GLOBAL_CONST chi::u32 kUpgrade = 5;
  
  // Bdev-specific methods (start from 10)
  GLOBAL_CONST chi::u32 kAllocate = 10;
  GLOBAL_CONST chi::u32 kFree = 11;
  GLOBAL_CONST chi::u32 kWrite = 12;
  GLOBAL_CONST chi::u32 kRead = 13;
  GLOBAL_CONST chi::u32 kGetStats = 14;
  
  // Target registration methods
  GLOBAL_CONST chi::u32 kRegisterTarget = 15;
  GLOBAL_CONST chi::u32 kUnregisterTarget = 16;
  GLOBAL_CONST chi::u32 kListTargets = 17;
}

} // namespace chimaera::bdev_extended

#endif // BDEV_AUTOGEN_METHODS_H_
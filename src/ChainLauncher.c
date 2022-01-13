// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#include <orbis/Sysmodule.h>
#include <orbis/SystemService.h>
#include <stddef.h>

#include "libLog.h"

// This is here because some mod menus just scan for a running "eboot.bin" and attach automatically...
// So we have to account for their poor programming by not running as "eboot.bin" ourselves
int main() {
  logSetLogLevel(LL_All);

  if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE) != 0) {
    logKernel(LL_Fatal, "%s", "Unable to load ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE");
    return -1;
  }

  logKernel(LL_Info, "%s", "Loading \"/app0/guest.elf\"");
  sceSystemServiceLoadExec("/app0/guest.elf", NULL);

  return 0;
}

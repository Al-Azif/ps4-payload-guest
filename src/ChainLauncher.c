#include <orbis/SystemService.h>

#include <stddef.h>

// This is here because some mod menus just scan for a running `eboot.bin` and attach automatically...
// So we have to account for their poor programming by not running as `eboot.bin` ourselves
int main() {
  sceSystemServiceLoadExec("/app0/guest.elf", NULL);

  return 0;
}

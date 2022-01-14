// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#include "App.h"

#include <orbis/Sysmodule.h>     // sceSysmoduleLoadModuleInternal
#include <orbis/SystemService.h> // sceSystemServiceLoadExec
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstddef> // NULL

#include "libLog.h"
#include "notifi.h"

#include "Utility.h" // g_Shellcode and Utility::memoryProtectedDestroy()

extern "C" {
extern int backtrace(void *p_Array, int p_Len);
}

// TODO: Add to SDK
#define ORBIS_SYSMODULE_INTERNAL_SYSUTIL 0x80000026
#define ORBIS_SYSMODULE_INTERNAL_libSceGnmDriver 0x80000052

void signalHandler(int p_SignalNum) {
  void *s_Array[100];

  // TODO: Maybe do some other handling later
  switch (p_SignalNum) {
  case 11:
    logKernel(LL_Debug, "%s", "SIGSEGV");
    break;
  default:
    break;
  }

  backtrace(s_Array, 100);

  if (g_Shellcode != NULL) {
    Utility::MemoryProtectedDestroy(g_Shellcode);
    notifi(NULL, "The Payload you attempted to load is corrupted or has crashed\nPlease pick a different one");
  }

  sceSystemServiceLoadExec("/app0/guest.elf", NULL);
}

int main() {
  // Start the Application
  if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE) != 0) {
    return -1;
  }

  if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_VIDEO_OUT) != 0) {
    return -1;
  }

  if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET) != 0) {
    return -1;
  }

  if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE) != 0) {
    return -1;
  }

  if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_PAD) != 0) {
    return -1;
  }

  if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSUTIL) != 0) {
    return -1;
  }

  if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_libSceGnmDriver) != 0) {
    return -1;
  }

  sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NETCTL);
  sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET);
  sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_HTTP);
  sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SSL);

  // Signal handler
  struct sigaction s_NewSigAction;
  s_NewSigAction.sa_handler = signalHandler;
  sigemptyset(&s_NewSigAction.sa_mask);
  s_NewSigAction.sa_flags = 0;

  for (int i = 0; i < 43; i++) {
    sigaction(i, &s_NewSigAction, NULL);
  }

  // Main application
  Application *s_MainApp = new Application();
  sceSystemServiceHideSplashScreen();
  s_MainApp->Run();
  delete s_MainApp;

  // "Clean" close
  sceSystemServiceLoadExec("exit", NULL);

  return 0;
}

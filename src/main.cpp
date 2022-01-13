#include "App.h"

#include "libLog.h"
#include "notifi.h"

#include "Utility.h" // g_Shellcode and Utility::memoryProtectedDestroy()

#include <cstddef>               // NULL
#include <cstdlib>               // std::free()
#include <orbis/SystemService.h> // sceSystemServiceLoadExec

#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

void signalHandler(int p_SignalNum) {
  // TODO: Maybe do some other handling later
  switch (p_SignalNum) {
  case 11:
    logKernel(LL_Debug, "%s", "SIGSEGV");
  default:
    break;
  }

  if (g_Shellcode != NULL) {
    Utility::memoryProtectedDestroy(g_Shellcode);
    std::free(g_Shellcode);
    notifi(NULL, "The Payload you attempted to load is corrupted or has crashed\nPlease pick a different one");
  }

  sceSystemServiceLoadExec("/app0/guest.elf", NULL);
}

int main() {
  // Start the Application

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

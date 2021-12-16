#include "App.h"

#include <orbis/SystemService.h> // sceSystemServiceLoadExec

#include <cstddef> // NULL

int main() {
  // Start the Application
  Application *s_MainApp = new Application();
  sceSystemServiceHideSplashScreen();
  s_MainApp->Run();
  delete s_MainApp;

  // "Clean" close
  sceSystemServiceLoadExec("exit", NULL);

  return 0;
}

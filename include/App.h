#ifndef APPLICATION_H
#define APPLICATION_H

#include "View.h"

#include "libjbc.h"

#include <string>

#define APP_VER 0.96

class Language;
class Controller;
class Graphics;
class Resource;

class Application {
public:
  Language *Lang;
  Controller *Ctrl;
  Graphics *Graph;
  Resource *Res;

  Application();
  ~Application();
  void Run();
  void Update();
  void Render();
  void ChangeView(View *p_View);
  void ShowFatalReason(const std::string &p_Reason);

private:
  bool m_IsRunning;
  View *m_CurrentView;
  int m_FlipArgs;

  jbc_cred m_Cred;
  jbc_cred m_RootCreds;

  bool IsJailbroken();
  void Jailbreak();
  void Unjailbreak();
};

#endif

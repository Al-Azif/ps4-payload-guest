// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#ifndef APPLICATION_H
#define APPLICATION_H

#include "View.h"

#include <string>

#define APP_VER 0.97

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
};

#endif

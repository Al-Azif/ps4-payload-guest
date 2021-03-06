// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#include "App.h"

#include <orbis/Sysmodule.h>

#include <cstdio>
#include <string>

#include "libLog.h"

#include "Controller.h"
#include "Graphics.h"
#include "Language.h"
#include "PayloadsView.h"
#include "Resource.h"
#include "View.h"

// #define SCE_SYSMDOULE_LIBIME 0x0095
// #define SCE_SYSMODULE_IME_DIALOG 0x0096

Application::Application() {
  m_IsRunning = false;

  // Initialize system componant
  logKernel(LL_Debug, "%s", "Initialize Language...");
  Lang = new Language();
  logKernel(LL_Debug, "%s", "Initialize Controller...");
  Ctrl = new Controller();
  logKernel(LL_Debug, "%s", "Initialize Graphics...");
  Graph = new Graphics();
  logKernel(LL_Debug, "%s", "Initialize Resource...");
  Res = new Resource(this, Lang->GetTypefacePaths());

  // Load some needed module
  // sceSysmoduleLoadModule(SCE_SYSMDOULE_LIBIME);
  // sceSysmoduleLoadModule(SCE_SYSMODULE_IME_DIALOG);

  // Setup the default view
  logKernel(LL_Debug, "%s", "Initialize the Main View...");

  PayloadsView *s_MainView = new PayloadsView(this);
  m_CurrentView = s_MainView;
}

Application::~Application() {
  // Cleanup
  logKernel(LL_Debug, "%s", "Cleanup Application...");

  delete Lang;
  delete Res;
  delete Graph;
  delete Ctrl;
}

void Application::Update() {
  Ctrl->Update();

  if (m_CurrentView) {
    m_CurrentView->Update();
  } else {
    logKernel(LL_Debug, "%s", "No view setup!");
  }
}

void Application::ShowFatalReason(const std::string &p_Reason) {
  while (1) {
    Graph->WaitFlip();

    int s_ScreenHeight = Graph->GetScreenHeight();
    int s_ScreenWidth = Graph->GetScreenWidth();

    Color s_Red = {0xFF, 0x00, 0x00, 0xFF};
    Color s_White = {0xFF, 0xFF, 0xFF, 0xFF};

    // Draw red background
    Graph->DrawRectangle(0, 0, s_ScreenWidth, s_ScreenHeight, s_Red);

    // Draw message
    std::string s_Message = std::string(Lang->Get("errorFatal") + std::string("\n\n") + std::string(p_Reason) + std::string("\n\n\n\n") + Lang->Get("errorCloseApplication"));
    FontSize s_MessageSize;
    Graph->SetFontSize(Res->m_Typefaces, 45);
    Graph->GetTextSize(s_Message, Res->m_Typefaces, &s_MessageSize);
    Graph->DrawText(s_Message, Res->m_Typefaces, ((s_ScreenWidth / 2) - (s_MessageSize.width / 2)), ((s_ScreenHeight / 2) - (s_MessageSize.height / 2)), s_White, s_White);
    Graph->SetFontSize(Res->m_Typefaces, DEFAULT_FONT_SIZE);

    Graph->SwapBuffer(m_FlipArgs);
    m_FlipArgs++;
  }
}

void Application::Render() {
  Graph->WaitFlip();

  if (m_CurrentView) {
    m_CurrentView->Render();
  } else {
    logKernel(LL_Debug, "%s", "No view setup!");
  }

  Graph->SwapBuffer(m_FlipArgs);
  m_FlipArgs++;
}

void Application::ChangeView(View *p_View) {
  logKernel(LL_Debug, "Change view to %p!", (void *)p_View);

  if (!p_View) {
    return;
  }

  m_CurrentView = p_View;
}

void Application::Run() {
  logKernel(LL_Debug, "%s", "Application: RUN");

  m_FlipArgs = 0;
  m_IsRunning = true;

  // Main Loop
  while (m_IsRunning) {
    Update();
    Render();
  }
}

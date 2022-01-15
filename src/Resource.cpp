// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#include "Resource.h"

#include <proto-include.h>

#include <cstdio>
#include <vector>

#include "libLog.h"

#include "App.h"
#include "Graphics.h"

// Initialize resources
Resource::Resource(Application *p_App, std::vector<std::string> p_TypefacePaths) {
  m_App = p_App;
  if (!m_App) {
    logKernel(LL_Debug, "%s", "App is null!");
    return;
  }

  // Load images
  if (m_App->Graph->LoadPNG(&m_Logo, "/app0/assets/images/Logo.png") < 0) {
    logKernel(LL_Debug, "%s", "Unable to load logo image!");
  }
  m_Logo.use_alpha = false;

  if (m_App->Graph->LoadPNG(&m_Unknown, "/app0/assets/images/UnknownImage.png") < 0) {
    logKernel(LL_Debug, "%s", "Unable to load unknown image!");
  }
  m_Unknown.use_alpha = false;

  if (m_App->Graph->LoadPNG(&m_Lock, "/app0/assets/images/Lock.png") < 0) {
    logKernel(LL_Debug, "%s", "Unable to load lock image!");
  }
  m_Unknown.use_alpha = false;

  if (m_App->Graph->LoadPNG(&m_Cross, "/app0/assets/images/Cross.png") < 0) {
    logKernel(LL_Debug, "%s", "Unable to load cross image!");
  }
  m_Cross.use_alpha = false;

  if (m_App->Graph->LoadPNG(&m_Triangle, "/app0/assets/images/Triangle.png") < 0) {
    logKernel(LL_Debug, "%s", "Unable to load triangle image!");
  }
  m_Triangle.use_alpha = false;

  if (m_App->Graph->LoadPNG(&m_Square, "/app0/assets/images/Square.png") < 0) {
    logKernel(LL_Debug, "%s", "Unable to load square image!");
  }
  m_Square.use_alpha = false;

  if (m_App->Graph->LoadPNG(&m_Circle, "/app0/assets/images/Circle.png") < 0) {
    logKernel(LL_Debug, "%s", "Unable to load circle image!");
  }
  m_Circle.use_alpha = false;

  if (m_App->Graph->InitTypefaces(m_Typefaces, p_TypefacePaths, DEFAULT_FONT_SIZE) < 0) {
    logKernel(LL_Debug, "%s", "Failed to initialize typefaces!");
  }
}

// Freeing resources
Resource::~Resource() {
  if (!m_App) {
    logKernel(LL_Debug, "%s", "App is null!");
    return;
  }

  m_App->Graph->UnloadPNG(&m_Logo);
  m_App->Graph->UnloadPNG(&m_Unknown);
  m_App->Graph->UnloadPNG(&m_Lock);
  m_App->Graph->UnloadPNG(&m_Cross);
  m_App->Graph->UnloadPNG(&m_Triangle);
  m_App->Graph->UnloadPNG(&m_Square);
  m_App->Graph->UnloadPNG(&m_Circle);
}

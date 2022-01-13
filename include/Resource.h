// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#ifndef RESOURCE_H
#define RESOURCE_H

#include "App.h"
#include "Graphics.h"

#include <proto-include.h>

#include <vector>

class Application;

class Resource {
public:
  Resource(Application *p_App, std::vector<std::string> p_TypefacePaths);
  ~Resource();

  // Application definition
  Application *m_App;

  // Image
  Image m_Logo;
  Image m_Unknown;
  Image m_Lock;
  Image m_Cross;
  Image m_Triangle;
  Image m_Square;
  Image m_Circle;

  // Font faces
  std::vector<FT_Face> m_Typefaces;
};

#endif

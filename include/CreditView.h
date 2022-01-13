// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#ifndef CREDIT_VIEW_H
#define CREDIT_VIEW_H

#include "App.h"
#include "Graphics.h"
#include "View.h"

class Application;

class CreditView : public View {
public:
  CreditView(Application *p_App);
  virtual ~CreditView();

  int Update(void);
  int Render(void);

private:
  Application *m_App;

  // Colors
  Color m_BackgroundColor;
  Color m_TextColor;
  Color m_DividerColor;
};

#endif

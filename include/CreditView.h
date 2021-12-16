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

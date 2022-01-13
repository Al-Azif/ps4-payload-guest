// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#include "CreditView.h"

#include "App.h"
#include "Controller.h"
#include "Graphics.h"
#include "Language.h"
#include "PayloadsView.h"
#include "Resource.h"
#include "Utility.h"

#include "libLog.h"

#include <string>

#define HEADER_SIZE 200
#define FOOTER_SIZE 100
#define BORDER_X 100

// Load the credit view value
CreditView::CreditView(Application *p_App) {
  logKernel(LL_Debug, "%s", "CreditView: Loaded!");

  m_App = p_App;
  if (!m_App) {
    logKernel(LL_Debug, "%s", "App is null!");
    return;
  }

  // Setup color
  m_BackgroundColor = {0x0, 0x28, 0x87, 0xFF};
  m_TextColor = {0xFF, 0xFF, 0xFF, 0xFF};
  m_DividerColor = {0xFF, 0xFF, 0xFF, 0xFF};

  logKernel(LL_Debug, "%s", "CreditView: Initialized!");
}

CreditView::~CreditView() {
}

int CreditView::Update() {
  if (m_App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_CIRCLE)) {
    PayloadsView *s_ReturnView = new PayloadsView(m_App);
    m_App->ChangeView(s_ReturnView);
    delete this;
    return 0;
  }

  return 0;
}

int CreditView::Render() {
  int s_ScreenHeight = m_App->Graph->GetScreenHeight();
  int s_ScreenWidth = m_App->Graph->GetScreenWidth();

  // Draw background color
  m_App->Graph->DrawRectangle(0, 0, s_ScreenWidth, s_ScreenHeight, m_BackgroundColor);

  // Draw logo
  int s_LogoY = (HEADER_SIZE / 2) - (120 / 2);
  m_App->Graph->DrawPNG(&m_App->Res->m_Logo, BORDER_X, s_LogoY);

  // Draw credits title
  std::string s_TitleName = m_App->Lang->Get("creditsTitle");
  FontSize s_TitleSize;
  m_App->Graph->SetFontSize(m_App->Res->m_Typefaces, 54);
  m_App->Graph->GetTextSize(s_TitleName, m_App->Res->m_Typefaces, &s_TitleSize);
  m_App->Graph->DrawText(s_TitleName, m_App->Res->m_Typefaces, BORDER_X + 120 + BORDER_X, (s_LogoY + ((120 / 2) - (s_TitleSize.height / 2))), m_TextColor, m_TextColor);

  // Draw header divider
  m_App->Graph->DrawRectangle(BORDER_X, HEADER_SIZE - 5, s_ScreenWidth - (BORDER_X * 2), 5, m_DividerColor);

  m_App->Graph->SetFontSize(m_App->Res->m_Typefaces, DEFAULT_FONT_SIZE);

  std::string s_CreditText;
  FontSize s_CreditSize;
  Utility::StrSprintf(s_CreditText, "Al Azif: This PKG\n\nTheoryWrong: UI Base\n\nOpenOrbis Team: Mira & Toolchain\n\nnlohmann: JSON Library\n\nVersion: %.2f", APP_VER);
  m_App->Graph->GetTextSize(s_CreditText, m_App->Res->m_Typefaces, &s_CreditSize);
  m_App->Graph->DrawText(s_CreditText, m_App->Res->m_Typefaces, (s_ScreenWidth / 2) - (s_CreditSize.width / 2), (s_ScreenHeight / 2) - (s_CreditSize.height / 2), m_TextColor, m_TextColor);

  // Draw footer divider
  m_App->Graph->DrawRectangle(BORDER_X, s_ScreenHeight - FOOTER_SIZE + 5, s_ScreenWidth - (BORDER_X * 2), 5, m_DividerColor);

  m_App->Graph->SetFontSize(m_App->Res->m_Typefaces, FOOTER_FONT_SIZE);

  std::string s_ReturnMain = m_App->Lang->Get("return");
  FontSize s_ReturnMainSize;
  m_App->Graph->GetTextSize(s_ReturnMain, m_App->Res->m_Typefaces, &s_ReturnMainSize);

  int s_IconWidth = (s_ReturnMainSize.width) + (1 * FOOTER_ICON_SIZE) + (1 * FOOTER_TEXT_BORDER);

  int s_IconX = (s_ScreenWidth - BORDER_X) - s_IconWidth;
  int s_IconY = (s_ScreenHeight - FOOTER_SIZE + 5) + FOOTER_BORDER_Y;
  int s_TextY = s_IconY + ((FOOTER_ICON_SIZE / 2) - (s_ReturnMainSize.height / 2));

  m_App->Graph->DrawSizedPNG(&m_App->Res->m_Circle, s_IconX, s_IconY, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
  s_IconX += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
  m_App->Graph->DrawText(s_ReturnMain, m_App->Res->m_Typefaces, s_IconX, s_TextY, m_TextColor, m_TextColor);

  m_App->Graph->SetFontSize(m_App->Res->m_Typefaces, DEFAULT_FONT_SIZE);

  return 0;
}

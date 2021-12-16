#ifndef SOURCES_VIEW_H
#define SOURCES_VIEW_H

#include "App.h"
#include "Graphics.h"
#include "Utility.h"
#include "View.h"

#include <orbis/_types/pthread.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class Application;
class PayloadsView;

typedef struct {
  std::string name;
  std::string location;
  Image icon;
} Payload;

class PayloadsView : public View {
public:
  PayloadsView(Application *p_App);
  virtual ~PayloadsView();
  int Update(void);
  int Render(void);

private:
  Application *m_App;

  OrbisPthreadMutex m_PayloadsMtx;

  std::vector<Payload> m_Payloads;
  size_t m_PayloadSelected;

  std::string m_ErrorMessage;
  bool m_OnError;

  // Fetch animation
  int m_FetchStepAnim;
  uint64_t m_FetchTimeStep;

  // Payload timer
  uint64_t m_PayloadTimer;

  // Colors
  Color m_BackgroundColor;
  Color m_TextColor;
  Color m_SubtextColor;
  Color m_DividerColor;
  Color m_InactiveColor;
  Color m_ActiveColor;

  template <typename... Args>
  void ShowError(const std::string &p_Format, Args... p_Args) {
    Utility::StrSprintf(m_ErrorMessage, p_Format, p_Args...);
    m_OnError = true;
  }
  std::vector<Payload> ParseJson(const std::string &p_Path);
  std::vector<Payload> ParseFiles(const std::string &p_Path);
  void ParseDirectory(const std::string &p_Path);
  void RefreshPayloadList();
  void CleanupPayloadList();
};

#endif

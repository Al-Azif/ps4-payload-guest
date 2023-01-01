// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#ifndef SOURCES_VIEW_H
#define SOURCES_VIEW_H

#include <orbis/_types/pthread.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "App.h"
#include "Graphics.h"
#include "Utility.h"
#include "View.h"

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

  void RefreshPayloadList(bool p_Reset);
  void CleanupPayloadList(bool p_Reset);

  bool sendPayloads(int port = 9090) {
    if (m_Payloads.size() > 0) {
      if (m_PayloadTimer == 0 || m_PayloadTimer + (5 * 1000000) < sceKernelGetProcessTime()) {
        logKernel(LL_Debug, "Loading: %s", m_Payloads[m_PayloadSelected].location.c_str());
        // notifi(NULL, "Loading: %s", m_Payloads[m_PayloadSelected].location.c_str()); // Pop notification
        if (!Utility::SendPayload(m_App, "127.0.0.1", port, m_Payloads[m_PayloadSelected].location)) { // Send to GoldHEN's loader
          Utility::LaunchShellcode(m_App, m_Payloads[m_PayloadSelected].location);                     // Launch here
        }
        m_PayloadTimer = sceKernelGetProcessTime();
        return true;
      } else {
        logKernel(LL_Debug, "Skip loading due to timer: %s", m_Payloads[m_PayloadSelected].location.c_str());
        return false;
      }
    }
    return false;
  }
};

#endif

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#include "PayloadsView.h"

#include <dirent.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "libLog.h"
// #include "notifi.h"

#include "App.h"
#include "Controller.h"
#include "CreditView.h"
#include "Graphics.h"
#include "Language.h"
#include "Resource.h"
#include "Utility.h"
#include "nlohmann_json.hpp"

std::vector<Payload> PayloadsView::ParseJson(const std::string &p_Path) {
  std::vector<Payload> s_TempOutput;

  FILE *s_MetaFilePointer = std::fopen(p_Path.c_str(), "r");
  if (!s_MetaFilePointer) {
    return s_TempOutput;
  }

  std::fseek(s_MetaFilePointer, 0, SEEK_END);
  size_t s_Filesize = std::ftell(s_MetaFilePointer);
  std::fseek(s_MetaFilePointer, 0, SEEK_SET);

  std::vector<char> s_RawJson; // TODO: Vector, char array, or calloc?
  s_RawJson.reserve(s_Filesize + 1);
  std::memset(&s_RawJson[0], '\0', s_Filesize + 1);

  if (std::fread(&s_RawJson[0], s_Filesize, sizeof(char), s_MetaFilePointer) != s_Filesize) {
    ShowError(std::string(m_App->Lang->Get("errorMetadataRead") + std::string("\n\n") + m_App->Lang->Get("errorPathOutput")), p_Path.c_str());
    return s_TempOutput;
  }

  std::fclose(s_MetaFilePointer);

  std::string s_FullPath = std::string(p_Path);
  s_FullPath.erase(s_FullPath.size() - 10); // Removes `/meta.json`

  nlohmann::json s_Data = nlohmann::json::parse(&s_RawJson[0], nullptr, false, true);
  if (s_Data.is_discarded() || !s_Data.is_array()) {
    ShowError(std::string(m_App->Lang->Get("errorMetadata") + std::string("\n\n") + m_App->Lang->Get("errorMetadataMalformed") + std::string("\n\n") + m_App->Lang->Get("errorPathOutput")), p_Path.c_str());
    return s_TempOutput;
  }

  for (size_t i = 0; i < s_Data.size(); i++) {
    if (!s_Data[i].contains("name")) {
      ShowError(std::string(m_App->Lang->Get("errorMetadata") + std::string("\n\n") + m_App->Lang->Get("errorMetadataMissingName") + std::string("\n\n") + m_App->Lang->Get("errorPathOutput") + std::string("\n\n") + m_App->Lang->Get("errorIndexOutput")), p_Path.c_str(), i);
      return s_TempOutput;
    }

    if (!s_Data[i].contains("filename")) {
      ShowError(std::string(m_App->Lang->Get("errorMetadata") + std::string("\n\n") + m_App->Lang->Get("errorMetadataMissingFilename") + std::string("\n\n") + m_App->Lang->Get("errorPathOutput") + std::string("\n\n") + m_App->Lang->Get("errorIndexOutput")), p_Path.c_str(), i);
      return s_TempOutput;
    }

    Payload s_TempPayload;
    std::memset(&s_TempPayload, '\0', sizeof(s_TempPayload));

    s_TempPayload.name = s_Data[i]["name"].get<std::string>();
    Utility::SanitizeJsonString(s_TempPayload.name);

    if (s_TempPayload.name.size() <= 0) {
      ShowError(std::string(m_App->Lang->Get("errorMetadata") + std::string("\n\n") + m_App->Lang->Get("errorMetadataNullName") + std::string("\n\n") + m_App->Lang->Get("errorPathOutput") + std::string("\n\n") + m_App->Lang->Get("errorIndexOutput")), p_Path.c_str(), i);
      return s_TempOutput;
    }

    s_TempPayload.location = std::string(s_FullPath) + std::string("/") + s_Data[i]["filename"].get<std::string>();
    Utility::SanitizeJsonString(s_TempPayload.location);

    // Check if filepath exists and is a file (TODO: stat is currently broken in the SDK)
    DIR *s_DirTest = opendir(s_TempPayload.location.c_str());
    if (s_DirTest) {
      closedir(s_DirTest);
      ShowError(std::string(m_App->Lang->Get("errorMetadata") + std::string("\n\n") + m_App->Lang->Get("errorMetadataMissingPayloadFile") + std::string("\n\n") + m_App->Lang->Get("errorPathOutput") + std::string("\n\n") + m_App->Lang->Get("errorIndexOutput")), p_Path.c_str(), i);
      return s_TempOutput;
    }
    FILE *s_TestFilePointer = std::fopen(s_TempPayload.location.c_str(), "r");
    if (!s_TestFilePointer) {
      ShowError(std::string(m_App->Lang->Get("errorMetadata") + std::string("\n\n") + m_App->Lang->Get("errorMetadataMissingPayloadFile") + std::string("\n\n") + m_App->Lang->Get("errorPathOutput") + std::string("\n\n") + m_App->Lang->Get("errorIndexOutput")), p_Path.c_str(), i);
      return s_TempOutput;
    }
    std::fclose(s_TestFilePointer);

    std::string s_IconPath;
    if (s_Data[i].contains("icon")) {
      s_IconPath = std::string(s_FullPath) + std::string("/") + s_Data[i]["icon"].get<std::string>();
      if (m_App->Graph->LoadPNG(&s_TempPayload.icon, s_IconPath) >= 0) {
        s_TempPayload.icon.use_alpha = false;
      }
    }

    s_TempOutput.push_back(s_TempPayload);
  }

  return s_TempOutput;
}

std::vector<Payload> PayloadsView::ParseFiles(const std::string &p_Path) {
  std::vector<Payload> s_TempOutput;

  DIR *s_Dir = opendir(p_Path.c_str());
  if (s_Dir) {
    struct dirent *s_Entry;
    while ((s_Entry = readdir(s_Dir)) != NULL) {
      // Skip anything that is not a `.bin`
      if (s_Entry->d_namlen < 5) {
        continue;
      }
      std::string s_Filename = std::string(s_Entry->d_name).substr(0, s_Entry->d_namlen - 4);
      std::string s_Extension = Utility::LastChars(std::string(s_Entry->d_name), 4);
      if (strncasecmp(s_Extension.c_str(), ".bin", 4) != 0) {
        continue;
      }

      // Skip things that are not regular files
      std::string s_FullPath = std::string(p_Path) + std::string("/") + std::string(s_Filename) + std::string(s_Extension);
      if (s_Entry->d_type != DT_REG) {
        continue;
      }

      Payload s_TempPayload;
      std::memset(&s_TempPayload, '\0', sizeof(s_TempPayload));
      s_TempPayload.name = std::string(s_Filename);
      s_TempPayload.location = std::string(s_FullPath);

      // Make image path
      DIR *s_PngDir = opendir(p_Path.c_str());
      if (s_PngDir) {
        struct dirent *s_PngEntry;
        while ((s_PngEntry = readdir(s_PngDir)) != NULL) {
          if (s_PngEntry->d_type != DT_REG) {
            continue;
          }

          std::string icon_path = std::string(p_Path) + std::string("/") + std::string(s_PngEntry->d_name);
          if (strncasecmp(s_PngEntry->d_name, std::string(std::string(s_Filename) + std::string(".png")).c_str(), sizeof(s_PngEntry->d_name)) == 0) {
            if (m_App->Graph->LoadPNG(&s_TempPayload.icon, icon_path) >= 0) {
              s_TempPayload.icon.use_alpha = false;
              break;
            }
          }
        }
      }

      s_TempOutput.push_back(s_TempPayload);
    }
    closedir(s_Dir);
  }

  return s_TempOutput;
}

void PayloadsView::ParseDirectory(const std::string &p_Path) {
  DIR *s_Dir = opendir(p_Path.c_str());
  if (s_Dir) {
    closedir(s_Dir);
    std::string s_MetaPath = std::string(p_Path) + std::string("/meta.json"); // TODO: Verify this is a file and not just a directory (TODO: stat is currently broken in the SDK)
    FILE *s_MetaFilePointer = std::fopen(s_MetaPath.c_str(), "r");
    std::vector<Payload> s_TempOutput;
    if (s_MetaFilePointer) {
      std::fclose(s_MetaFilePointer);
      s_TempOutput = ParseJson(s_MetaPath);
    } else {
      s_TempOutput = ParseFiles(p_Path);
    }
    m_Payloads.insert(m_Payloads.end(), s_TempOutput.begin(), s_TempOutput.end());
  }
}

void PayloadsView::RefreshPayloadList(bool p_Reset) {
  logKernel(LL_Debug, "%s", "GeneratePayloads is called!");

  scePthreadMutexLock(&m_PayloadsMtx);

  CleanupPayloadList(p_Reset);

  // Check internal path
  ParseDirectory("/data/payloads");

  // Check USB paths
  for (int i = 0; i < 8; i++) {
    std::string s_UsbPath;
    if (Utility::IsJailbroken()) {
      Utility::StrSprintf(s_UsbPath, "/mnt/usb%i/payloads", i);
    } else {
      Utility::StrSprintf(s_UsbPath, "/usb%i/payloads", i);
    }
    ParseDirectory(s_UsbPath);
  }

  logKernel(LL_Debug, "%s", "GeneratePayloads: Refreshed");

  scePthreadMutexUnlock(&m_PayloadsMtx);
}

void PayloadsView::CleanupPayloadList(bool p_Reset) {
  logKernel(LL_Debug, "%s", "Cleanup payload list...");

  for (size_t i = 0; i < m_Payloads.size(); i++) {
    if (m_Payloads[i].icon.img != NULL) {
      m_App->Graph->UnloadPNG(&m_Payloads[i].icon);
    }
  }

  if (p_Reset) {
    m_PayloadSelected = 0;
  }
  m_Payloads.clear();
}

PayloadsView::PayloadsView(Application *p_App) {
  m_App = p_App;
  if (!m_App) {
    logKernel(LL_Debug, "%s", "App is null!");
    return;
  }

  // Setup variables
  m_OnError = false;

  m_FetchStepAnim = 0;
  m_FetchTimeStep = 0;

  m_PayloadTimer = 0;

  // Setup color
  m_BackgroundColor = {0x00, 0x28, 0x87, 0xFF};
  m_TextColor = {0xFF, 0xFF, 0xFF, 0xFF};
  m_SubtextColor = {0x90, 0x90, 0x90, 0xFF};
  m_DividerColor = {0xFF, 0xFF, 0xFF, 0xFF}; // Premix color if not fully opaque
  m_InactiveColor = m_BackgroundColor;       // Premix color if not fully opaque
  m_ActiveColor = {0x5C, 0x74, 0xB0, 0xFF};  // Premix color if not fully opaque

  scePthreadMutexInit(&m_PayloadsMtx, NULL, "PayloadGuestMTX");
  RefreshPayloadList(true);

  logKernel(LL_Debug, "%s", "PayloadsView: End of constructor");
}

PayloadsView::~PayloadsView() {
  // Cleanup memory
  scePthreadMutexLock(&m_PayloadsMtx);
  CleanupPayloadList(true);
  scePthreadMutexUnlock(&m_PayloadsMtx);

  // Destroy mutex
  scePthreadMutexDestroy(&m_PayloadsMtx);
}

int PayloadsView::Update() {
  // If mutex is locked, payloads list generation is in progress
  if (scePthreadMutexTrylock(&m_PayloadsMtx) >= 0) {
    if (m_OnError) {
      if (m_App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_CROSS)) {
        m_OnError = false;
      }
    } else {
      if (m_App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_UP)) {
        if (m_PayloadSelected != 0) {
          m_PayloadSelected--;
        }
      }

      if (m_App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_L1)) {
        size_t s_NewSelected = m_PayloadSelected;
        if (s_NewSelected >= MENU_NBR_PER_PAGE) {
          s_NewSelected -= MENU_NBR_PER_PAGE;
        }
        s_NewSelected -= s_NewSelected % MENU_NBR_PER_PAGE;
        m_PayloadSelected = s_NewSelected;
      }

      if (m_App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_DOWN)) {
        if ((m_PayloadSelected + 1) < m_Payloads.size()) {
          m_PayloadSelected++;
        }
      }

      if (m_App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_R1)) {
        size_t s_NewSelected = (m_PayloadSelected / MENU_NBR_PER_PAGE) * MENU_NBR_PER_PAGE + MENU_NBR_PER_PAGE;
        if ((s_NewSelected + 1) >= m_Payloads.size()) {
          m_PayloadSelected = m_Payloads.size() - 1;
        } else {
          m_PayloadSelected = s_NewSelected;
        }
      }

      if (m_App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_TRIANGLE)) {
        CreditView *cdt_view = new CreditView(m_App);
        m_App->ChangeView(cdt_view);
        scePthreadMutexUnlock(&m_PayloadsMtx);
        delete this;
        return 0;
      }

      if (m_App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_SQUARE)) {
        RefreshPayloadList(true);
        return 0;
      }

      if (m_App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_CROSS)) {
        bool is_payload_sent = sendPayloads(9090);
        if (is_payload_sent) {
          RefreshPayloadList(false);
        }
      }

      if (m_App->Ctrl->GetButtonPressed(ORBIS_PAD_BUTTON_CIRCLE)) {
        bool is_payload_sent = sendPayloads(9021);
        if (is_payload_sent) {
          RefreshPayloadList(false);
        }
      }
    }

    // Unlock the mutex
    scePthreadMutexUnlock(&m_PayloadsMtx);
  }

  return 0;
}

int PayloadsView::Render() {
  int s_ScreenHeight = m_App->Graph->GetScreenHeight();
  int s_ScreenWidth = m_App->Graph->GetScreenWidth();

  // Calculate drawable package
  int s_MenuX = BORDER_X;
  int s_MenuY = HEADER_SIZE + MENU_BORDER;
  int s_MenuYEnd = s_ScreenHeight - FOOTER_SIZE;

  int s_MenuSizeHeight = s_MenuYEnd - s_MenuY;
  int s_MenuSizeWidth = s_ScreenWidth - (BORDER_X * 2);

  int s_ConsumableBorder = (MENU_BORDER * (MENU_NBR_PER_PAGE - 2));
  int s_SourceLineHeight = (s_MenuSizeHeight - s_ConsumableBorder) / MENU_NBR_PER_PAGE;

  int s_Delta = m_PayloadSelected / MENU_NBR_PER_PAGE;
  int s_MaxPage;
  if (m_Payloads.size() % MENU_NBR_PER_PAGE == 0) {
    s_MaxPage = m_Payloads.size() / MENU_NBR_PER_PAGE;
  } else {
    s_MaxPage = m_Payloads.size() / MENU_NBR_PER_PAGE + 1;
  }

  std::string s_CurrentPageString;
  Utility::StrSprintf(s_CurrentPageString, m_App->Lang->Get("pageIndicator"), (s_Delta + 1), s_MaxPage);

  // Draw background color
  m_App->Graph->DrawRectangle(0, 0, s_ScreenWidth, s_ScreenHeight, m_BackgroundColor);

  // Draw logo
  int s_LogoY = (HEADER_SIZE / 2) - (120 / 2);
  m_App->Graph->DrawPNG(&m_App->Res->m_Logo, BORDER_X, s_LogoY);

  // Draw title
  std::string s_TitleName = m_App->Lang->Get("mainTitle");
  FontSize s_TitleSize;
  m_App->Graph->SetFontSize(m_App->Res->m_Typefaces, HEADER_FONT_SIZE);
  m_App->Graph->GetTextSize(s_TitleName, m_App->Res->m_Typefaces, &s_TitleSize);
  m_App->Graph->DrawText(s_TitleName, m_App->Res->m_Typefaces, BORDER_X + 120 + BORDER_X, (s_LogoY + ((120 / 2) - (s_TitleSize.height / 2))), m_TextColor, m_TextColor);

  // Draw header divider
  m_App->Graph->DrawRectangle(BORDER_X, HEADER_SIZE - 5, s_ScreenWidth - (BORDER_X * 2), 5, m_DividerColor);

  m_App->Graph->SetFontSize(m_App->Res->m_Typefaces, DEFAULT_FONT_SIZE);

  // If mutex is locked, fetch is in progress
  if (scePthreadMutexTrylock(&m_PayloadsMtx) < 0) {
    uint64_t s_AnimationTime = sceKernelGetProcessTime();

    // stepAnim go from 0 to x each seconds
    if ((m_FetchTimeStep + 1000000) <= s_AnimationTime) {
      m_FetchTimeStep = s_AnimationTime;
      m_FetchStepAnim++;
      if (m_FetchStepAnim > 3) {
        m_FetchStepAnim = 0;
      }
    }

    std::string progress;
    if (m_FetchStepAnim == 0) {
      progress = m_App->Lang->Get("loadingTick1");
    } else if (m_FetchStepAnim == 1) {
      progress = m_App->Lang->Get("loadingTick2");
    } else if (m_FetchStepAnim == 2) {
      progress = m_App->Lang->Get("loadingTick3");
    }

    // Show progress ticker
    FontSize s_ProgressSize;
    m_App->Graph->GetTextSize(progress, m_App->Res->m_Typefaces, &s_ProgressSize);

    int s_TextX = ((s_ScreenWidth / 2) - (s_ProgressSize.width / 2));
    int s_TextY = ((s_ScreenHeight / 2) - (s_ProgressSize.height / 2));
    m_App->Graph->DrawText(progress, m_App->Res->m_Typefaces, s_TextX, s_TextY, m_TextColor, m_TextColor);

    // Draw footer divider
    m_App->Graph->DrawRectangle(BORDER_X, s_ScreenHeight - FOOTER_SIZE + 5, s_ScreenWidth - (BORDER_X * 2), 5, m_DividerColor);
  } else {
    m_FetchStepAnim = 0;

    // Mutex now locked, draw packages list
    if (m_OnError) {
      m_App->Graph->SetFontSize(m_App->Res->m_Typefaces, DEFAULT_FONT_SIZE);
      FontSize s_ErrorSize;
      m_App->Graph->GetTextSize(m_ErrorMessage, m_App->Res->m_Typefaces, &s_ErrorSize);
      m_App->Graph->DrawText(m_ErrorMessage, m_App->Res->m_Typefaces, ((s_ScreenWidth / 2) - (s_ErrorSize.width / 2)), ((s_ScreenHeight / 2) - (s_ErrorSize.height / 2)), m_TextColor, m_TextColor);
    } else if (m_Payloads.size() > 0) {
      // Draw payloads
      for (int l_MenuPosition = 0; l_MenuPosition < MENU_NBR_PER_PAGE; l_MenuPosition++) {
        size_t s_CurrentPayload = (s_Delta * MENU_NBR_PER_PAGE) + l_MenuPosition;
        if (s_CurrentPayload >= m_Payloads.size()) {
          break;
        }

        Color s_Selection = m_InactiveColor;
        if (m_PayloadSelected == s_CurrentPayload) {
          s_Selection = m_ActiveColor;
        }

        int s_CurrentMenuY = s_MenuY + (l_MenuPosition * s_SourceLineHeight) + (l_MenuPosition * MENU_BORDER);
        m_App->Graph->DrawRectangle(s_MenuX, s_CurrentMenuY, s_MenuSizeWidth, s_SourceLineHeight, s_Selection);

        int s_IconSize = s_SourceLineHeight - (2 * MENU_IN_IMAGE_BORDER);
        if (m_Payloads[s_CurrentPayload].icon.img != NULL) {
          m_App->Graph->DrawSizedPNG(&m_Payloads[s_CurrentPayload].icon, s_MenuX + MENU_IN_IMAGE_BORDER, s_CurrentMenuY + MENU_IN_IMAGE_BORDER, s_IconSize, s_IconSize);
        } else {
          m_App->Graph->DrawSizedPNG(&m_App->Res->m_Unknown, s_MenuX + MENU_IN_IMAGE_BORDER, s_CurrentMenuY + MENU_IN_IMAGE_BORDER, s_IconSize, s_IconSize);
        }

        if (m_Payloads[s_CurrentPayload].name.size() > 0) {
          // We should ALWAYS make into this conditional
          m_App->Graph->SetFontSize(m_App->Res->m_Typefaces, DEFAULT_FONT_SIZE);
          FontSize s_TitleSize;
          m_App->Graph->GetTextSize(m_Payloads[s_CurrentPayload].name, m_App->Res->m_Typefaces, &s_TitleSize);
          m_App->Graph->DrawText(m_Payloads[s_CurrentPayload].name, m_App->Res->m_Typefaces, s_MenuX + s_IconSize + (3 * MENU_IN_IMAGE_BORDER), s_CurrentMenuY + ((s_SourceLineHeight / 2) - (s_TitleSize.height / 2)), m_TextColor, m_TextColor);
        }

        if (m_Payloads[s_CurrentPayload].location.size() > 0) {
          // We should ALWAYS make into this conditional
          m_App->Graph->SetFontSize(m_App->Res->m_Typefaces, SUBTEXT_FONT_SIZE);
          FontSize s_LocationSize;
          m_App->Graph->GetTextSize(m_Payloads[s_CurrentPayload].location, m_App->Res->m_Typefaces, &s_LocationSize);
          m_App->Graph->DrawText(m_Payloads[s_CurrentPayload].location, m_App->Res->m_Typefaces, s_MenuX + s_IconSize + (3 * MENU_IN_IMAGE_BORDER) + 30, s_CurrentMenuY + ((s_SourceLineHeight / 2) - (s_LocationSize.height / 2)) + 36, m_SubtextColor, m_SubtextColor);
        }
      }
    } else {
      m_App->Graph->SetFontSize(m_App->Res->m_Typefaces, DEFAULT_FONT_SIZE);
      std::string s_Ohno = m_App->Lang->Get("errorNoPayloads");
      FontSize s_OhnoSize;
      m_App->Graph->GetTextSize(s_Ohno, m_App->Res->m_Typefaces, &s_OhnoSize);
      m_App->Graph->DrawText(s_Ohno, m_App->Res->m_Typefaces, ((s_ScreenWidth / 2) - (s_OhnoSize.width / 2)), ((s_ScreenHeight / 2) - (s_OhnoSize.height / 2)), m_TextColor, m_TextColor);
    }

    // Draw footer divider
    m_App->Graph->DrawRectangle(BORDER_X, s_ScreenHeight - FOOTER_SIZE + 5, s_ScreenWidth - (BORDER_X * 2), 5, m_DividerColor);

    m_App->Graph->SetFontSize(m_App->Res->m_Typefaces, FOOTER_FONT_SIZE);
    if (m_OnError) {
      std::string ok = m_App->Lang->Get("ok");
      FontSize okSize;
      m_App->Graph->GetTextSize(ok, m_App->Res->m_Typefaces, &okSize);

      int icon_width = okSize.width + FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
      int icon_x = (s_ScreenWidth - BORDER_X) - icon_width;
      int icon_y = (s_ScreenHeight - FOOTER_SIZE + 5) + FOOTER_BORDER_Y;
      int text_y = icon_y + ((FOOTER_ICON_SIZE / 2) - (okSize.height / 2));

      m_App->Graph->DrawSizedPNG(&m_App->Res->m_Cross, icon_x, icon_y, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
      icon_x += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
      m_App->Graph->DrawText(ok, m_App->Res->m_Typefaces, icon_x, text_y, m_TextColor, m_TextColor);
    } else {
      std::string select = m_App->Lang->Get("select");
      std::string selectMira = m_App->Lang->Get("selectMira");
      std::string refresh = m_App->Lang->Get("refresh");
      std::string credit = m_App->Lang->Get("credits");

      FontSize selectSize;
      FontSize selectMiraSize;
      FontSize refreshSize;
      FontSize creditSize;

      m_App->Graph->SetFontSize(m_App->Res->m_Typefaces, FOOTER_FONT_SIZE);

      m_App->Graph->GetTextSize(select, m_App->Res->m_Typefaces, &selectSize);
      m_App->Graph->GetTextSize(selectMira, m_App->Res->m_Typefaces, &selectMiraSize);
      m_App->Graph->GetTextSize(refresh, m_App->Res->m_Typefaces, &refreshSize);
      m_App->Graph->GetTextSize(credit, m_App->Res->m_Typefaces, &creditSize);

      int s_IconWidth = (selectSize.width + selectMiraSize.width + refreshSize.width + creditSize.width) + (4 * FOOTER_ICON_SIZE) + (4 * FOOTER_TEXT_BORDER);
      int s_IconX = (s_ScreenWidth - BORDER_X) - s_IconWidth;
      int s_IconY = (s_ScreenHeight - FOOTER_SIZE + 5) + FOOTER_BORDER_Y;
      int s_TextY = s_IconY + ((FOOTER_ICON_SIZE / 2) - (selectSize.height / 2));

      if (m_PayloadTimer == 0 || m_PayloadTimer + (5 * 1000000) < sceKernelGetProcessTime()) {
        m_App->Graph->DrawSizedPNG(&m_App->Res->m_Cross, s_IconX, s_IconY, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
      } else {
        m_App->Graph->DrawSizedPNG(&m_App->Res->m_Lock, s_IconX, s_IconY, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
      }
      s_IconX += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
      m_App->Graph->DrawText(select, m_App->Res->m_Typefaces, s_IconX, s_TextY, m_TextColor, m_TextColor);
      s_IconX += selectSize.width + FOOTER_FONT_SIZE;

      if (m_PayloadTimer == 0 || m_PayloadTimer + (5 * 1000000) < sceKernelGetProcessTime()) {
        m_App->Graph->DrawSizedPNG(&m_App->Res->m_Circle, s_IconX, s_IconY, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
      } else {
        m_App->Graph->DrawSizedPNG(&m_App->Res->m_Lock, s_IconX, s_IconY, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
      }
      s_IconX += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
      m_App->Graph->DrawText(selectMira, m_App->Res->m_Typefaces, s_IconX, s_TextY, m_TextColor, m_TextColor);
      s_IconX += selectMiraSize.width + FOOTER_FONT_SIZE;

      m_App->Graph->DrawSizedPNG(&m_App->Res->m_Square, s_IconX, s_IconY, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
      s_IconX += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
      m_App->Graph->DrawText(refresh, m_App->Res->m_Typefaces, s_IconX, s_TextY, m_TextColor, m_TextColor);
      s_IconX += refreshSize.width + FOOTER_FONT_SIZE;

      m_App->Graph->DrawSizedPNG(&m_App->Res->m_Triangle, s_IconX, s_IconY, FOOTER_ICON_SIZE, FOOTER_ICON_SIZE);
      s_IconX += FOOTER_ICON_SIZE + FOOTER_TEXT_BORDER;
      m_App->Graph->DrawText(credit, m_App->Res->m_Typefaces, s_IconX, s_TextY, m_TextColor, m_TextColor);
      // s_IconX += creditSize.width + FOOTER_FONT_SIZE;

      // Draw page number
      if (m_Payloads.size() > 0) {
        FontSize s_PageSize;
        m_App->Graph->GetTextSize(s_CurrentPageString, m_App->Res->m_Typefaces, &s_PageSize);
        m_App->Graph->DrawText(s_CurrentPageString, m_App->Res->m_Typefaces, BORDER_X, s_TextY, m_TextColor, m_TextColor);
      }
    }

    m_App->Graph->SetFontSize(m_App->Res->m_Typefaces, DEFAULT_FONT_SIZE);

    // Unlock the mutex
    scePthreadMutexUnlock(&m_PayloadsMtx);
  }

  return 0;
}

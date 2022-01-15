// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#include "Language.h"

#include <orbis/SystemService.h>

#include <cstdint>
#include <string>
#include <vector>

#include "libLog.h"
#include "nlohmann_json.hpp"

#include "Utility.h"

Language::Language() {
  m_Code = GetCode();
  m_Name = GetName();
  m_TypefacePaths = GetTypefacePaths();
  m_Data = GetData();

  if (m_Data.is_discarded() || m_Data.is_null()) {
    logKernel(LL_Debug, "%s", "Failed to load language attempting to fallback to English (United States)");
    m_Data = Fallback();
  }

  // TODO: If data still empty `IsInitialized` should be false
}

Language::~Language() {
  m_Data.clear();
  m_TypefacePaths.clear();
  m_Name.clear();
  m_Code = -1;

  // TODO:`IsInitialized` set to false
}

std::string Language::Get(const std::string &p_Key) {
  if (!m_Data.contains(p_Key) || m_Data[p_Key].get<std::string>().empty()) {
    if (!m_Data.contains("unknownKey") || m_Data["unknownKey"].get<std::string>().empty()) {
      return "[UNTRANSLATED TEXT]";
    }
    return m_Data["unknownKey"].get<std::string>();
  }
  return m_Data[p_Key].get<std::string>();
}

int32_t Language::GetCode() {
  int32_t s_Code;

  if (sceSystemServiceParamGetInt(ORBIS_SYSTEM_SERVICE_PARAM_ID_LANG, &s_Code) != 0) {
    logKernel(LL_Debug, "%s", "Unable to query language code");
    return 0x1;
  }

  return s_Code;
}

std::string Language::GetName() {
  std::string s_Name;

  switch (m_Code) {
  case 0:
    s_Name = "Japanese";
    break;
  case 1:
    s_Name = "English (United States)";
    break;
  case 2:
    s_Name = "French (France)";
    break;
  case 3:
    s_Name = "Spanish (Spain)";
    break;
  case 4:
    s_Name = "German";
    break;
  case 5:
    s_Name = "Italian";
    break;
  case 6:
    s_Name = "Dutch";
    break;
  case 7:
    s_Name = "Portuguese (Portugal)";
    break;
  case 8:
    s_Name = "Russian";
    break;
  case 9:
    s_Name = "Korean";
    break;
  case 10:
    s_Name = "Chinese (Traditional)";
    break;
  case 11:
    s_Name = "Chinese (Simplified)";
    break;
  case 12:
    s_Name = "Finnish";
    break;
  case 13:
    s_Name = "Swedish";
    break;
  case 14:
    s_Name = "Danish";
    break;
  case 15:
    s_Name = "Norwegian";
    break;
  case 16:
    s_Name = "Polish";
    break;
  case 17:
    s_Name = "Portuguese (Brazil)";
    break;
  case 18:
    s_Name = "English (United Kingdom)";
    break;
  case 19:
    s_Name = "Turkish";
    break;
  case 20:
    s_Name = "Spanish (Latin America)";
    break;
  case 21:
    s_Name = "Arabic";
    break;
  case 22:
    s_Name = "French (Canada)";
    break;
  case 23:
    s_Name = "Czech";
    break;
  case 24:
    s_Name = "Hungarian";
    break;
  case 25:
    s_Name = "Greek";
    break;
  case 26:
    s_Name = "Romanian";
    break;
  case 27:
    s_Name = "Thai";
    break;
  case 28:
    s_Name = "Vietnamese";
    break;
  case 29:
    s_Name = "Indonesian";
    break;
  default:
    s_Name = "UNKNOWN";
    break;
  }

  return s_Name;
}

std::vector<std::string> Language::GetTypefacePaths() {
  std::vector<std::string> s_TypefacePath;

  switch (m_Code) {
  case 0:
    s_TypefacePath.push_back("/app0/assets/fonts/NotoSansJP-Regular.ttf");
    break;
  case 9:
    s_TypefacePath.push_back("/app0/assets/fonts/NotoSansKR-Regular.ttf");
    break;
  case 10:
    s_TypefacePath.push_back("/app0/assets/fonts/NotoSansTC-Regular.ttf");
    break;
  case 11:
    s_TypefacePath.push_back("/app0/assets/fonts/NotoSansSC-Regular.ttf");
    break;
  case 21:
    s_TypefacePath.push_back("/app0/assets/fonts/NotoSansArabic-Regular.ttf");
    break;
  case 27:
    s_TypefacePath.push_back("/app0/assets/fonts/NotoSansThai-Regular.ttf");
    break;
  default:
    break;
  }

  s_TypefacePath.push_back("/app0/assets/fonts/NotoSans-Regular.ttf"); // This is the fallback so we at least have an ASCII table

  return s_TypefacePath;
}

nlohmann::json Language::GetData() {
  nlohmann::json s_Data;
  std::string s_LanguagePath;
  Utility::StrSprintf(s_LanguagePath, "/app0/assets/languages/%02i.json", m_Code);

  FILE *s_LanguageFilePointer = std::fopen(s_LanguagePath.c_str(), "r");
  if (!s_LanguageFilePointer) {
    logKernel(LL_Debug, "Unable to open language file: %s", s_LanguagePath.c_str());
    return s_Data;
  }

  std::fseek(s_LanguageFilePointer, 0, SEEK_END);
  size_t s_LanguageFilesize = std::ftell(s_LanguageFilePointer);
  std::fseek(s_LanguageFilePointer, 0, SEEK_SET);

  std::vector<char> s_RawJson; // TODO: Vector, char array, or calloc?
  s_RawJson.reserve(s_LanguageFilesize + 1);
  std::memset(&s_RawJson[0], '\0', s_LanguageFilesize + 1);

  if (std::fread(&s_RawJson[0], s_LanguageFilesize, sizeof(char), s_LanguageFilePointer) < 0) {
    logKernel(LL_Debug, "Unable to read language file: %s", s_LanguagePath.c_str());
    return s_Data;
  }

  std::fclose(s_LanguageFilePointer);

  s_Data = nlohmann::json::parse(&s_RawJson[0], nullptr, false, true);
  if (s_Data.is_discarded() || !s_Data.is_object()) {
    logKernel(LL_Debug, "Malformed language file: %s", s_LanguagePath.c_str());
    s_Data.clear();
    return s_Data;
  }

  return s_Data;
}

nlohmann::json Language::Fallback() {
  m_Code = 0x1;
  m_Name = GetName();
  m_TypefacePaths = GetTypefacePaths();
  return GetData();
}

// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#ifndef UTILITY_H
#define UTILITY_H

#include "App.h"

// #include <orbis/_types/ime_dialog.h>

#include <cstddef>
#include <memory>
#include <string>

typedef struct {
  void *writable;
  void *executable;
  size_t size;
} MemoryProtected;

extern MemoryProtected *g_Shellcode;

class Application;

class Utility {
public:
  static std::string LastChars(std::string p_Input, int p_Num);
  static void SanitizeJsonString(std::string &p_Input);
  static std::wstring StrToWstr(const std::string &p_Input);

  static bool IsJailbroken();

  static int MemoryProtectedCreate(MemoryProtected **p_Memory, size_t p_Size);
  static int MemoryProtectedDestroy(MemoryProtected *p_Memory);
  static int MemoryProtectedGetWritableAddress(MemoryProtected *p_Memory, void **p_Address);
  static int MemoryProtectedGetExecutableAddress(MemoryProtected *p_Memory, void **p_Address);
  static int MemoryProtectedGetSize(MemoryProtected *p_Memory, size_t *p_Size);

  static void LaunchShellcode(Application *p_App, const std::string &p_Path);
  static void SendPayload(Application *p_App, const std::string p_IpAddress, uint16_t p_Port, const std::string &p_PayloadPath);

  template <typename I>
  static I AlignUp(I p_Value, I p_Align) {
    auto s_maxValue = std::max(p_Value, p_Align);
    return (s_maxValue + (p_Align - 1)) & ~(p_Align - 1);
  }

  // https://stackoverflow.com/a/26221725
  template <typename... Args>
  static int StrSprintf(std::string &p_Output, const std::string &p_Format, Args... p_Args) {
    int s_SizeTemp = std::snprintf(nullptr, 0, p_Format.c_str(), p_Args...) + 1;
    if (s_SizeTemp <= 0) {
      return -1;
    }

    size_t s_Size = static_cast<size_t>(s_SizeTemp);
    auto s_Buffer = std::make_unique<char[]>(s_Size);
    std::snprintf(s_Buffer.get(), s_Size, p_Format.c_str(), p_Args...);
    p_Output = std::string(s_Buffer.get(), s_Buffer.get() + s_Size - 1);

    return p_Output.size();
  }

  // static int RemplaceText(const char *file, const char *text, const char *what);
  // static char *StrReplace(char *orig, char *rep, char *with);
  // static int CopyFile(char *source, char *destination);
  // static char *ReadFile(const char *path, size_t *size);
  // static int CleanupMap(void *data, size_t data_size);
  // static int AppendText(const char *file, const char *line);
  // static char *OpenKeyboard(OrbisImeType type, const char *title);
  // static bool GetFWVersion(char *version);
  // static bool ByteToHex(char *buf, size_t buf_size, const void *data, size_t data_size);
  // static int WriteFile(const char *file, const char *buffer, size_t size);
};

#endif

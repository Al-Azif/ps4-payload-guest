#include "Utility.h"

#include "Language.h"

#include "libLog.h"
#include "notifi.h"

// #include <orbis/ImeDialog.h>
#include <orbis/libkernel.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

MemoryProtected *g_Shellcode;

std::string Utility::LastChars(std::string p_Input, int p_Num) {
  int s_InputSize = p_Input.size();
  return (p_Num > 0 && s_InputSize > p_Num) ? p_Input.substr(s_InputSize - p_Num) : "";
}

// https://stackoverflow.com/a/34221488
void Utility::SanitizeJsonString(std::string &p_Input) {
  // Add backslashes.
  for (auto i = p_Input.begin();;) {
    auto const s_Position = find_if(i, p_Input.end(), [](char const c) { return '\\' == c || '\'' == c || '"' == c; });
    if (s_Position == p_Input.end()) {
      break;
    }
    i = next(p_Input.insert(s_Position, '\\'), 2);
  }

  p_Input.erase(remove_if(p_Input.begin(), p_Input.end(), [](char const c) { return '\a' == c || '\b' == c || '\f' == c || '\n' == c || '\r' == c || '\t' == c || '\v' == c || '\?' == c || '\0' == c || '\x1A' == c; }), p_Input.end());
}

// https://github.com/Mish7913/cpp-library/blob/master/str/str.cpp
std::wstring Utility::StrToWstr(const std::string &p_Input) {
  std::string s_CurrentLocale = std::setlocale(LC_ALL, "");
  const char *s_CharSource = p_Input.c_str();
  size_t s_CharDestinationSize = std::mbstowcs(NULL, s_CharSource, 0) + 1;
  wchar_t *s_CharDestination = new wchar_t[s_CharDestinationSize];
  std::wmemset(s_CharDestination, 0, s_CharDestinationSize);
  std::mbstowcs(s_CharDestination, s_CharSource, s_CharDestinationSize);
  std::wstring s_Result = s_CharDestination;
  delete[] s_CharDestination;
  std::setlocale(LC_ALL, s_CurrentLocale.c_str());
  return s_Result;
}

int Utility::memoryProtectedCreate(MemoryProtected **p_Memory, size_t p_Size) {
  MemoryProtected *s_Mem;
  long s_PageSize = sysconf(_SC_PAGESIZE);

  if (p_Memory == NULL) {
    return -1;
  }

  if (p_Size == 0) {
    return -1;
  }

  s_Mem = (MemoryProtected *)std::calloc(sizeof(char), sizeof(MemoryProtected));
  if (s_Mem == NULL) {
    return -1;
  }

  // Align to s_PageSize
  s_Mem->size = (p_Size / s_PageSize + 1) * s_PageSize;

  s_Mem->executable = mmap(NULL, s_Mem->size, 7, 0x1000 | 0x08000, -1, 0);
  if (s_Mem->executable == MAP_FAILED) {
    std::free(s_Mem);
    return -1;
  }

  s_Mem->writable = s_Mem->executable;
  if (s_Mem->writable == MAP_FAILED) {
    std::free(s_Mem);
    return -1;
  }
  *p_Memory = s_Mem;

  return 0;
}

int Utility::memoryProtectedDestroy(MemoryProtected *p_Memory) {
  if (p_Memory == NULL) {
    return -1;
  }

  int s_Ret = 0;

  s_Ret |= munmap(p_Memory->writable, p_Memory->size);
  s_Ret |= munmap(p_Memory->executable, p_Memory->size);
  std::free(p_Memory);

  return s_Ret;
}

int Utility::memoryProtectedGetWritableAddress(MemoryProtected *p_Memory, void **p_Address) {
  if (p_Memory == NULL || p_Address == NULL) {
    return -1;
  }

  *p_Address = p_Memory->writable;

  return 0;
}

int Utility::memoryProtectedGetExecutableAddress(MemoryProtected *p_Memory, void **p_Address) {
  if (p_Memory == NULL || p_Address == NULL) {
    return -1;
  }

  *p_Address = p_Memory->executable;

  return 0;
}

int Utility::memoryProtectedGetSize(MemoryProtected *p_Memory, size_t *p_Size) {
  if (p_Memory == NULL || p_Size == NULL) {
    return -1;
  }

  *p_Size = p_Memory->size;

  return 0;
}

void Utility::LaunchShellcode(Application *p_App, const std::string &p_Path) {
  Application *s_App = p_App;

  if (!s_App) {
    logKernel(LL_Debug, "%s", "Invalid application object");
    return;
  }

  int s_PayloadFileDescriptor = open(p_Path.c_str(), O_RDONLY);
  if (s_PayloadFileDescriptor < 0) {
    notifi(NULL, s_App->Lang->Get("errorShellcodeOpen").c_str(), p_Path.c_str());
    return;
  }
  size_t s_Filesize = lseek(s_PayloadFileDescriptor, 0, SEEK_END);
  lseek(s_PayloadFileDescriptor, 0, SEEK_SET);

  g_Shellcode = (MemoryProtected *)std::calloc(sizeof(char), sizeof(MemoryProtected));
  if (g_Shellcode == NULL) {
    close(s_PayloadFileDescriptor);
    notifi(NULL, s_App->Lang->Get("errorSendPayloadBuffer").c_str(), p_Path.c_str());

    return;
  }
  if (memoryProtectedCreate(&g_Shellcode, 0x100000) != 0) {
    close(s_PayloadFileDescriptor);
    std::free(g_Shellcode);
    notifi(NULL, s_App->Lang->Get("errorShellcodeMprotect").c_str(), p_Path.c_str());

    return;
  }

  void *s_Writable;
  void *s_Executable;

  memoryProtectedGetWritableAddress(g_Shellcode, &s_Writable);
  logKernel(LL_Debug, "memoryProtectedGetWritableAddress writable=%p", s_Writable);

  memoryProtectedGetExecutableAddress(g_Shellcode, &s_Executable);
  logKernel(LL_Debug, "memoryProtectedGetExecutableAddress executable=%p", s_Executable);

  unsigned char *s_Buffer = (unsigned char *)std::calloc(sizeof(char), s_Filesize);
  if (s_Buffer == NULL) {
    close(s_PayloadFileDescriptor);
    std::free(g_Shellcode);
    notifi(NULL, s_App->Lang->Get("errorSendPayloadBuffer").c_str(), p_Path.c_str());

    return;
  }

  ssize_t s_Ret = read(s_PayloadFileDescriptor, s_Buffer, s_Filesize);
  close(s_PayloadFileDescriptor);

  if (s_Ret < 0 || s_Ret != s_Filesize) { // Checks for negative first because s_ret is signed
    std::free(g_Shellcode);
    std::free(s_Buffer);
    notifi(NULL, s_App->Lang->Get("errorSendPayloadRead").c_str(), p_Path.c_str());

    return;
  }

  std::memcpy(s_Writable, s_Buffer, s_Filesize);
  std::free(s_Buffer);

  pthread_t s_Loader;
  pthread_create(&s_Loader, NULL, (void *(*)(void *))s_Executable, NULL);
  // pthread_join(s_Loader, NULL);

  logKernel(LL_Debug, "%s", "Exiting LaunchShellcode()");
}

void Utility::SendPayload(Application *p_App, const std::string p_IpAddress, uint16_t p_Port, const std::string &p_PayloadPath) {
  Application *s_App = p_App;

  if (!s_App) {
    logKernel(LL_Debug, "%s", "Invalid application object");
    return;
  }

  FILE *s_PayloadFilePointer = std::fopen(p_PayloadPath.c_str(), "r");
  if (!s_PayloadFilePointer) {
    notifi(NULL, "%s", s_App->Lang->Get("errorSendPayloadOpen").c_str());
    return;
  }

  std::fseek(s_PayloadFilePointer, 0, SEEK_END);
  size_t s_PayloadFilesize = std::ftell(s_PayloadFilePointer);
  std::fseek(s_PayloadFilePointer, 0, SEEK_SET);

  void *s_PayloadBuffer = std::calloc(sizeof(char), s_PayloadFilesize);
  if (!s_PayloadBuffer) {
    std::fclose(s_PayloadFilePointer);
    notifi(NULL, "%s", s_App->Lang->Get("errorSendPayloadBuffer").c_str());
    return;
  }

  if (std::fread(s_PayloadBuffer, sizeof(char), s_PayloadFilesize, s_PayloadFilePointer) <= 0) {
    std::fclose(s_PayloadFilePointer);
    std::free(s_PayloadBuffer);
    notifi(NULL, "%s", s_App->Lang->Get("errorSendPayloadRead").c_str());
    return;
  }
  std::fclose(s_PayloadFilePointer);

  int s_SocketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (s_SocketFileDescriptor < 0) {
    std::free(s_PayloadBuffer);
    notifi(NULL, "%s", s_App->Lang->Get("errorSendPayloadCreateSocket").c_str());
    return;
  }

  struct sockaddr_in s_ServerAddress;
  std::memset(&s_ServerAddress, '\0', sizeof(s_ServerAddress));

  s_ServerAddress.sin_family = AF_INET;
  s_ServerAddress.sin_port = htons(p_Port);
  s_ServerAddress.sin_addr.s_addr = inet_addr(p_IpAddress.c_str());

  if (connect(s_SocketFileDescriptor, (struct sockaddr *)&s_ServerAddress, sizeof(s_ServerAddress)) < 0) {
    std::free(s_PayloadBuffer);
    notifi(NULL, "%s", s_App->Lang->Get("errorSendPayloadConnect").c_str());
    return;
  }

  if (send(s_SocketFileDescriptor, s_PayloadBuffer, s_PayloadFilesize, MSG_DONTWAIT) < 0) {
    close(s_SocketFileDescriptor);
    std::free(s_PayloadBuffer);
    notifi(NULL, "%s", s_App->Lang->Get("errorSendPayloadSend").c_str());
    return;
  }

  close(s_SocketFileDescriptor);
  std::free(s_PayloadBuffer);
}

// // Remplace a string inside a chain
// // Source: https://stackoverflow.com/questions/779875/what-function-is-to-replace-a-substring-from-a-string-in-c
// char *Utility::StrReplace(char *orig, char *rep, char *with) {
//   char *result;  // the return string
//   char *ins;     // the next insert point
//   char *tmp;     // varies
//   int len_rep;   // length of rep (the string to remove)
//   int len_with;  // length of with (the string to replace rep with)
//   int len_front; // distance between rep and end of last rep
//   int count;     // number of replacements

//   // sanity checks and initialization
//   if (!orig || !rep) {
//     return NULL;
//   }
//   len_rep = std::strlen(rep);
//   if (len_rep == 0) {
//     return NULL; // empty rep causes infinite loop during count
//   }

//   len_with = std::strlen(with);

//   // count the number of replacements needed
//   ins = orig;
//   for (count = 0; (tmp = std::strstr(ins, rep)); ++count) {
//     ins = tmp + len_rep;
//   }

//   tmp = result = (char *)std::calloc(sizeof(char), std::strlen(orig) + (len_with - len_rep) * count + 1);

//   if (!result) {
//     return NULL;
//   }

//   // first time through the loop, all the variable are set correctly
//   // from here on,
//   //    tmp points to the end of the result string
//   //    ins points to the next occurrence of rep in orig
//   //    orig points to the remainder of orig after "end of rep"
//   while (count--) {
//     ins = std::strstr(orig, rep);
//     len_front = ins - orig;
//     tmp = std::strncpy(tmp, orig, len_front) + len_front;
//     tmp = std::strcpy(tmp, with) + len_with;
//     orig += len_front + len_rep; // move to next "end of rep"
//   }
//   std::strcpy(tmp, orig);
//   return result;
// }

// // Copy file
// int Utility::CopyFile(char *source, char *destination) {
//   logKernel(LL_Debug, "Copying %s to %s...", source, destination);

//   int fd_src = sceKernelOpen(source, O_RDONLY, 0777);
//   int fd_dst = sceKernelOpen(destination, O_WRONLY | 0x0200 | 0x0400, 0777);
//   if (fd_src < 0 || fd_dst < 0) {
//     return 1;
//   }

//   int data_size = sceKernelLseek(fd_src, 0, SEEK_END);
//   sceKernelLseek(fd_src, 0, SEEK_SET);

//   void *data = NULL;

//   sceKernelMmap(0, data_size, PROT_READ, MAP_PRIVATE, fd_src, 0, &data);
//   sceKernelWrite(fd_dst, data, data_size);
//   sceKernelMunmap(data, data_size);

//   sceKernelClose(fd_dst);
//   sceKernelClose(fd_src);

//   return 0;
// }

// // Read file
// char *Utility::ReadFile(const char *path, size_t *size) {
//   logKernel(LL_Debug, "Reading %s...", path);

//   int fd_src = sceKernelOpen(path, 0x0000, 0777);
//   if (fd_src < 0) {
//     logKernel(LL_Debug, "Cannot open %s (0x%08x)", path, fd_src);
//     return NULL;
//   }

//   int data_size = sceKernelLseek(fd_src, 0, SEEK_END);
//   sceKernelLseek(fd_src, 0, SEEK_SET);

//   if (data_size <= 0) {
//     logKernel(LL_Debug, "Error with size (0x%08x)", data_size);
//     sceKernelClose(fd_src);
//     return NULL;
//   }

//   void *data = NULL;
//   sceKernelMmap(0, data_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd_src, 0, &data);
//   sceKernelClose(fd_src);

//   *size = data_size;

//   logKernel(LL_Debug, "sources.list data: %p", data);
//   logKernel(LL_Debug, "Data: %s (Size: %d)", data, data_size);

//   return (char *)data;
// }

// // Cleanup file data
// int Utility::CleanupMap(void *data, size_t data_size) {
//   logKernel(LL_Debug, "Cleanup map %p (size: %lu)", data, data_size);
//   return sceKernelMunmap(data, data_size);
// }

// // Append text on a file
// int Utility::AppendText(const char *file, const char *line) {
//   logKernel(LL_Debug, "Appening %s...", file);

//   int fd_dst = sceKernelOpen(file, 0x0002 | 0x0200, 0777);

//   if (fd_dst < 0) {
//     logKernel(LL_Debug, "Cannot open(write) %s (0x%08x)", file, fd_dst);
//     return 1;
//   }

//   int old_size = sceKernelLseek(fd_dst, 0, SEEK_END);
//   (void)old_size; // UNUSED

//   sceKernelWrite(fd_dst, "\n", 1);
//   sceKernelWrite(fd_dst, line, std::strlen(line));
//   sceKernelClose(fd_dst);

//   logKernel(LL_Debug, " [Done].");
//   return 0;
// }

// // Write a file
// int Utility::WriteFile(const char *file, const char *buffer, size_t size) {
//   logKernel(LL_Debug, "Writting %s...", file);

//   int fd_dst = sceKernelOpen(file, 0x0002 | 0x0200, 0777);

//   if (fd_dst < 0) {
//     logKernel(LL_Debug, "Cannot open(write) %s (0x%08x)", file, fd_dst);
//     return 1;
//   }

//   sceKernelWrite(fd_dst, buffer, size);
//   sceKernelClose(fd_dst);

//   logKernel(LL_Debug, " [Done].");
//   return 0;
// }

// // Remplace text on a file
// int Utility::RemplaceText(const char *file, const char *text, const char *what) {
//   logKernel(LL_Debug, "Remplace %s...", file);

//   size_t size;
//   char *data = Utility::ReadFile(file, &size);
//   if (!data) {
//     return 1;
//   }

//   char *final = Utility::StrReplace(data, (char *)text, (char *)what);
//   Utility::CleanupMap(data, size);

//   if (!final) {
//     return 2;
//   }

//   int fd_dst = sceKernelOpen(file, 0x0002 | 0x0200 | 0x0400, 0777);

//   if (fd_dst < 0) {
//     logKernel(LL_Debug, "Cannot open(write) %s (0x%08x)", file, fd_dst);
//     return 1;
//   }

//   sceKernelWrite(fd_dst, final, std::strlen(final));
//   sceKernelClose(fd_dst);

//   logKernel(LL_Debug, " [Done].");
//   std::free(final);
//   return 0;
// }

// // Open Keyboard (return a char on a calloc)
// char *Utility::OpenKeyboard(OrbisImeType type, const char *title) {
//   logKernel(LL_Debug, "Opening keyboard...");
//   OrbisImeDialogSetting param;
//   std::memset(&param, '\0', sizeof(param));

//   wchar_t bufferText[0x100];
//   wchar_t buffTitle[100];

//   std::memset(bufferText, '\0', sizeof(bufferText));
//   std::memset(buffTitle, '\0', sizeof(buffTitle));

//   std::mbstowcs(buffTitle, title, std::strlen(title) + 1);

//   param.maxTextLength = sizeof(bufferText);
//   param.inputTextBuffer = bufferText;
//   param.title = buffTitle;
//   param.userId = 254;
//   param.type = type;
//   param.enterLabel = ORBIS_BUTTON_LABEL_DEFAULT;
//   param.inputMethod = OrbisInput::ORBIS__DEFAULT;

//   int initDialog = sceImeDialogInit(&param, NULL);
//   (void)initDialog; // UNUSED

//   bool keyboardRunning = true;
//   while (keyboardRunning) {
//     int status = sceImeDialogGetStatus();

//     if (status == ORBIS_DIALOG_STATUS_STOPPED) {
//       OrbisDialogResult result;
//       std::memset(&result, '\0', sizeof(result));
//       sceImeDialogGetResult(&result);

//       if (result.endstatus == ORBIS_DIALOG_OK) {
//         logKernel(LL_Debug, "DIALOG OK");

//         char *finalText = (char *)std::calloc(sizeof(char), sizeof(bufferText) + 1);

//         if (!finalText) {
//           logKernel(LL_Debug, "OpenKeyboard: Error during calloc!");
//           return NULL;
//         }

//         std::wcstombs(finalText, bufferText, sizeof(bufferText));

//         logKernel(LL_Debug, "Text: %s", finalText);
//         keyboardRunning = false;
//         sceImeDialogTerm();

//         return finalText;
//       }

//       keyboardRunning = false;
//       sceImeDialogTerm();
//     } else if (status == ORBIS_DIALOG_STATUS_NONE) {
//       keyboardRunning = false;
//     }
//   }

//   return NULL;
// }

// // Get firmware version from libc (For prevent kernel change)
// bool Utility::GetFWVersion(char *version) {
//   char fw[2] = {0};
//   int fd = sceKernelOpen("/system/common/lib/libc.sprx", 0x0, 0777);
//   if (fd) {
//     sceKernelLseek(fd, 0x374, SEEK_CUR);
//     sceKernelRead(fd, &fw, 2);
//     sceKernelClose(fd);

//     sprintf(version, "%02x.%02x", fw[1], fw[0]);

//     return true;
//   } else {
//     logKernel(LL_Debug, "Failed to open libc!");
//     return false;
//   }
// }

// // Transform bytes to hex string
// bool Utility::ByteToHex(char *buf, size_t buf_size, const void *data, size_t data_size) {
//   static const char *digits = "0123456789ABCDEF";
//   const uint8_t *in = (const uint8_t *)data;
//   char *out = buf;
//   uint8_t c;
//   size_t i;
//   bool ret;

//   if (!buf || !data) {
//     ret = false;
//     goto err;
//   }
//   if (!buf_size || buf_size < (data_size * 2 + 1)) {
//     ret = false;
//     goto err;
//   }
//   if (!data_size) {
//     *out = '\0';
//     goto done;
//   }

//   for (i = 0; i < data_size; ++i) {
//     c = in[i];
//     *out++ = digits[c >> 4];
//     *out++ = digits[c & 0xF];
//   }
//   *out++ = '\0';

// done:
//   ret = true;

// err:
//   return ret;
// }

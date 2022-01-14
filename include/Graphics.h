// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// License: GPL-3.0

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <orbis/_types/kernel.h>
#include <orbis/_types/video.h>
#include <proto-include.h>

#include <cstddef>
#include <cstdint>
#include <string>

// Dimensions
#define FRAME_WIDTH 1920
#define FRAME_HEIGHT 1080
#define FRAME_DEPTH 4
#define FRAMEBUFFER_NBR 2

// Color is used to pack together RGB information, and is used for every function that draws colored pixels.
typedef struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} Color;

typedef struct Image {
  int width;
  int height;
  int channels;
  uint32_t *img;
  bool use_alpha;
} Image;

typedef struct FontSize {
  int width;
  int height;
} FontSize;

class Graphics {
public:
  Graphics();
  ~Graphics();

  // General
  void WaitFlip();
  void SwapBuffer(int p_FlipArgs);

  // Used for setting dimensions
  void SetDimensions(int p_Width, int p_Height, int p_Depth);
  int GetScreenWidth();
  int GetScreenHeight();

  void SetActiveFrameBuffer(int p_Index);

  // Initializes flip event queue and allocates video memory for frame buffers
  int InitializeFlipQueue(int p_Video);
  int AllocateVideoMem(size_t p_Size, int p_Alignment);
  void DeallocateVideoMem();
  int AllocateFrameBuffers(int p_Video, int p_Size);

  // Allocates display memory - bump allocator for video mem
  void *AllocDisplayMemBump(size_t p_Size);

  // Frame buffer functions
  void FrameWait(int p_Video, int64_t p_FlipArgs);
  void FrameBufferSwap();
  void FrameBufferClear();
  int64_t FrameBufferGetLastFlip();
  void FrameBufferSaveFlip(int64_t p_FlipArgs);
  void FrameBufferFill(Color p_Color);

  // Draw functions
  void DrawPixel(int p_X, int p_Y, Color p_Color);
  void DrawRectangle(int p_X, int p_Y, int p_Width, int p_Height, Color p_Color);

  // Typeface functions
  int InitTypefaces(std::vector<FT_Face> &p_Faces, std::vector<std::string> p_TypefacePaths, int p_FontSize);
  int SetFontSize(std::vector<FT_Face> &p_Faces, int p_FontSize);
  void DrawText(const std::string &p_Text, std::vector<FT_Face> p_Faces, int p_StartX, int p_StartY, Color p_BgColor, Color p_FgColor);
  void GetTextSize(const std::string &p_Text, std::vector<FT_Face> p_Faces, FontSize *p_Size);
  void DrawTextContainer(const std::string &p_Text, std::vector<FT_Face> p_Faces, int p_StartX, int p_StartY, int p_MaxWidth, int p_MaxHeight, Color p_BgColor, Color p_FgColor);

  // Image functions
  void DrawPNG(Image *p_Image, int p_StartX, int p_StartY);
  void DrawSizedPNG(Image *p_Image, int p_StartX, int p_StartY, int p_Width, int p_Height);
  int LoadPNG(Image *p_Image, const std::string &p_ImagePath);
  int LoadPNGFromMemory(Image *p_Image, unsigned char *p_ImageData, int p_ImageSize);
  void UnloadPNG(Image *p_Image);

private:
  FT_Library m_FtLib;

  // Video
  int m_Video;

  // Pointer to direct video memory and the frame buffer array
  void *m_VideoMem;
  void **m_FrameBuffers;

  // Points to the top of video memory, used for the bump allocator
  uintptr_t m_VideoMemSP;

  // Event queue for flips
  OrbisKernelEqueue m_FlipQueue;

  // Attributes for the frame buffers
  OrbisVideoOutBufferAttribute m_Attr;

  // Direct memory offset and allocation size
  off_t m_DirectMemOff;
  size_t m_DirectMemAllocationSize;

  // Meta info for frame information
  int m_Width;
  int m_Height;
  int m_Depth;

  // Frame buffer size and count - initialized by allocateFrameBuffers()
  int m_FrameBufferSize;
  int m_FrameBufferCount;

  // Active frame buffer for swapping
  int m_ActiveFrameBufferIdx;

  // flipArgLog
  int64_t *m_FlipArgLog;
};

#endif

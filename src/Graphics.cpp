#include "Graphics.h"

#include "Utility.h"

#include "libLog.h"

#include <orbis/GnmDriver.h>
#include <orbis/Sysmodule.h>
#include <orbis/VideoOut.h>
#include <orbis/libkernel.h>

#include <proto-include.h>

#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

//
/* alpha-blending, a simple vectorized version */
typedef unsigned int v4ui __attribute__((ext_vector_type(4)));
static inline uint32_t mix_color(const uint32_t *const p_Background, const uint32_t *const p_foreground) {
  uint32_t a = *p_foreground >> 24;

  if (a == 0) {
    return *p_Background;
  } else if (a == 0xFF) {
    return *p_foreground;
  }

  v4ui vin = (v4ui){*p_foreground, *p_foreground, *p_Background, *p_Background}; // vload
  v4ui vt = (v4ui){0x00FF00FF, 0x0000FF00, 0x00FF00FF, 0x0000FF00};              // vload

  vin &= vt;
  vt = (v4ui){a, a, 255 - a, 255 - a}; // vload, reuse t
  vin *= vt;

  vt = (v4ui){vin.x + vin.z, vin.y + vin.w, 0xFF00FF00, 0x00FF0000};
  vin = (v4ui){vt.x & vt.z, vt.y & vt.w};

  uint32_t Fg = a + ((*p_Background >> 24) * (255 - a) / 255);
  return (Fg << 24) | ((vin.x | vin.y) >> 8);
}

// Linearly interpolate x with y over s
inline float lerp(float p_X, float p_Y, float p_S) {
  return p_X * (1.0f - p_S) + p_Y * p_S;
}

Graphics::Graphics() {
  // Setup variable
  m_Width = FRAME_WIDTH;
  m_Height = FRAME_HEIGHT;
  m_Depth = FRAME_DEPTH;
  m_Video = -1;
  m_VideoMem = 0;
  m_ActiveFrameBufferIdx = 0;
  m_FrameBufferSize = 0;
  m_FrameBufferCount = 0;
  m_FrameBufferSize = m_Width * m_Height * m_Depth;

  int s_Ret;

  // Load freetype module
  s_Ret = sceSysmoduleLoadModule(0x009A);
  if (s_Ret < 0) {
    logKernel(LL_Debug, "%s", "Failed to load freetype module");
  }

  // Initialize freetype
  s_Ret = FT_Init_FreeType(&m_FtLib);
  if (s_Ret != 0) {
    logKernel(LL_Debug, "%s", "Failed to initialize freetypee");
  }

  // Open a handle to video out
  m_Video = sceVideoOutOpen(ORBIS_VIDEO_USER_MAIN, ORBIS_VIDEO_OUT_BUS_MAIN, 0, 0);
  if (m_Video < 0) {
    logKernel(LL_Debug, "%s", "Failed to open a video out handle");
  } else {
    logKernel(LL_Debug, "Handle is %i (0x%08x)", m_Video, m_Video);
  }

  // Create a queue for flip events
  if (InitializeFlipQueue(m_Video) < 0) {
    sceVideoOutClose(m_Video);

    logKernel(LL_Debug, "%s", "Failed to create an event queue");
  }

  // Allocate direct memory for the frame buffers
  s_Ret = AllocateVideoMem(0xC000000, 0x200000);

  if (s_Ret < 0) {
    sceVideoOutClose(m_Video);

    logKernel(LL_Debug, "%s", "Failed to allocate video memory");
  }

  // Set the frame buffers
  logKernel(LL_Debug, "%s", "Allocating framebuffer...");
  s_Ret = AllocateFrameBuffers(m_Video, FRAMEBUFFER_NBR);

  if (s_Ret < 0) {
    DeallocateVideoMem();
    sceVideoOutClose(m_Video);

    logKernel(LL_Debug, "%s", "Failed to allocate frame buffers from video memory");
  }

  SetActiveFrameBuffer(0);

  // Set the flip rate
  sceVideoOutSetFlipRate(m_Video, 0);
}

Graphics::~Graphics() {
  DeallocateVideoMem();
  sceVideoOutClose(m_Video);
}

void Graphics::WaitFlip() {
  // logKernel(LL_Debug, "%s", "WaitFlip...");

  FrameWait(m_Video, FrameBufferGetLastFlip() + 1);
  FrameBufferClear();
}

void Graphics::SwapBuffer(int p_FlipArgs) {
  sceGnmFlushGarlic();
  sceVideoOutSubmitFlip(m_Video, m_ActiveFrameBufferIdx, ORBIS_VIDEO_OUT_FLIP_VSYNC, p_FlipArgs);
  FrameBufferSaveFlip(p_FlipArgs);

  FrameBufferSwap();
}

// setActiveFrameBuffer takes the given index and sets it to the active index. Returns nothing.
void Graphics::SetActiveFrameBuffer(int p_Index) {
  m_ActiveFrameBufferIdx = p_Index;
}

// initializeFlipQueue creates an event queue for sceVideo flip events. Returns error code.
int Graphics::InitializeFlipQueue(int p_Video) {
  // Create an event queue
  int s_Ret = sceKernelCreateEqueue(&m_FlipQueue, "homebrew flip queue");

  if (s_Ret < 0) {
    return s_Ret;
  }

  // Unknown if we *have* to do this, but most ELFs seem to do this, so we'll do it too
  sceVideoOutAddFlipEvent(m_FlipQueue, p_Video, 0);
  return s_Ret;
}

// allocateVideoMem takes a given size and alignment, and allocates and maps direct memory and sets the VideoMem pointer to it.
// Returns error code.
int Graphics::AllocateVideoMem(size_t p_Size, int p_Alignment) {
  int s_Ret;

  // Align the allocation size
  m_DirectMemAllocationSize = (p_Size + p_Alignment - 1) / p_Alignment * p_Alignment;

  // Allocate memory for display buffer
  s_Ret = sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), m_DirectMemAllocationSize, p_Alignment, 3, &m_DirectMemOff);

  if (s_Ret < 0) {
    logKernel(LL_Debug, "sceKernelAllocateDirectMemory: an error occured. (%i => 0x%08x)", s_Ret, s_Ret);
    m_DirectMemAllocationSize = 0;
    return s_Ret;
  }

  logKernel(LL_Debug, "DirectMemOff: %p", (void *)m_DirectMemOff);
  logKernel(LL_Debug, "DirectMemAllocationSize: %p", (void *)m_DirectMemAllocationSize);
  logKernel(LL_Debug, "alignment: %i (0x%08x)", p_Alignment, p_Alignment);

  // Map the direct memory
  s_Ret = sceKernelMapDirectMemory(&m_VideoMem, m_DirectMemAllocationSize, 0x33, 0, m_DirectMemOff, p_Alignment);

  logKernel(LL_Debug, "VideoMem: %p", m_VideoMem);

  if (s_Ret < 0) {
    logKernel(LL_Debug, "sceKernelMapDirectMemory: an error occured. (%i => 0x%08x)", s_Ret, s_Ret);

    sceKernelReleaseDirectMemory(m_DirectMemOff, m_DirectMemAllocationSize);

    m_DirectMemOff = 0;
    m_DirectMemAllocationSize = 0;

    return s_Ret;
  }

  // Set the stack pointer to the beginning of the buffer
  m_VideoMemSP = (uintptr_t)m_VideoMem;

  return s_Ret;
}

// deallocateVideoMem is a function to be called upon error, and it'll free the mapped direct memory, zero out meta-data, and
// free the FrameBuffer array. Returns nothing.
void Graphics::DeallocateVideoMem() {
  // Free the direct memory
  sceKernelReleaseDirectMemory(m_DirectMemOff, m_DirectMemAllocationSize);

  // Zero out meta data
  m_VideoMem = 0;
  m_VideoMemSP = 0;
  m_DirectMemOff = 0;
  m_DirectMemAllocationSize = 0;

  // Free the frame buffer array
  std::free(m_FrameBuffers);
  m_FrameBuffers = 0;
}

// allocateFrameBuffer takes a given video handle and number of frames, and allocates an array with the give number of frames
// for the FrameBuffers, and also sets the attributes for the video handle. Finally, it registers the buffers to the handle
// before returning. Returns error code.
int Graphics::AllocateFrameBuffers(int p_Video, int p_Size) {
  m_FlipArgLog = (int64_t *)std::calloc(p_Size, sizeof(int64_t));

  // Allocate frame buffers array
  m_FrameBuffers = (void **)std::calloc(p_Size, sizeof(void *));
  if (!m_FrameBuffers) {
    logKernel(LL_Debug, "%s", "Unable to calloc FrameBuffers");
    return -1;
  }

  logKernel(LL_Debug, "handle: %i (0x%08x)", p_Video, p_Video);

  logKernel(LL_Debug, "flipArgLog: %p", (void *)m_FlipArgLog);
  logKernel(LL_Debug, "FrameBuffers: %p", (void *)m_FrameBuffers);

  // Allocate the framebuffer flip args log and set default value
  for (int l_DpBuffer = 0; l_DpBuffer < p_Size; l_DpBuffer++) {
    m_FlipArgLog[l_DpBuffer] = -2;
  }

  // Set the display buffers
  for (int i = 0; i < p_Size; i++) {
    m_FrameBuffers[i] = AllocDisplayMemBump(m_FrameBufferSize);
  }

  // Set SRGB pixel format
  sceVideoOutSetBufferAttribute(&m_Attr, 0x80000000, 1, 0, m_Width, m_Height, m_Width);
  int s_Ret = sceVideoOutRegisterBuffers(p_Video, 0, m_FrameBuffers, p_Size, &m_Attr);
  logKernel(LL_Debug, "sceVideoOutRegisterBuffers: 0x%08x", s_Ret);

  m_FrameBufferCount = p_Size;

  // Register the buffers to the video handle
  return s_Ret;
}

// allocDisplayMem is essentially a bump allocator, which will allocate space off of VideoMem for FrameBuffers. Returns a pointer
// to the chunk requested.
void *Graphics::AllocDisplayMemBump(size_t s_Size) {
  // Essentially just bump allocation
  void *s_AllocatedPtr = (void *)m_VideoMemSP;
  m_VideoMemSP += s_Size;

  return s_AllocatedPtr;
}

int64_t Graphics::FrameBufferGetLastFlip() {
  // logKernel(LL_Debug, "%s", "GetLastFlip");
  return m_FlipArgLog[m_ActiveFrameBufferIdx];
}

void Graphics::FrameBufferSaveFlip(int64_t p_FlipArgs) {
  // logKernel(LL_Debug, "%s", "SaveFlip");
  m_FlipArgLog[m_ActiveFrameBufferIdx] = p_FlipArgs;
}

// FrameBufferSwap swaps the ActiveFrameBufferIdx. Should be called at the end of every draw loop iteration. Returns nothing.
void Graphics::FrameBufferSwap() {
  // logKernel(LL_Debug, "%s", "BufferSwap");
  // Swap the frame buffer for some perf
  m_ActiveFrameBufferIdx = (m_ActiveFrameBufferIdx + 1) % m_FrameBufferCount;
}

// frameBufferClear fills the frame buffer with white pixels. Returns nothing.
void Graphics::FrameBufferClear() {
  // Clear the screen with a black frame buffer
  Color s_Blank = {0, 0, 0, 255};
  FrameBufferFill(s_Blank);
}

// frameBufferFill fills the frame buffer with pixels of the given red, green, and blue values. Returns nothing.
void Graphics::FrameBufferFill(Color p_Color) {
  // Draw row-by-row, column-by-column
  for (int yPos = 0; yPos < m_Height; yPos++) {
    for (int xPos = 0; xPos < m_Width; xPos++) {
      DrawPixel(xPos, yPos, p_Color);
    }
  }
}

// frameWait waits on the event queue before swapping buffers
void Graphics::FrameWait(int p_Video, int64_t p_FlipArgs) {
  // logKernel(LL_Debug, "%s", "FrameWait");
  OrbisKernelEvent s_Evt;
  int s_Count;

  // If the video handle is not initialized, bail out. This is mostly a failsafe, this should never happen.
  if (p_Video == 0) {
    return;
  }

  for (;;) {
    OrbisVideoOutFlipStatus s_FlipStatus;

    // Get the flip status and check the arg for the given frame ID
    sceVideoOutGetFlipStatus(p_Video, &s_FlipStatus);

    if (s_FlipStatus.flipArg >= p_FlipArgs) {
      break;
    }

    // Wait on next flip event
    if (sceKernelWaitEqueue(m_FlipQueue, &s_Evt, 1, &s_Count, 0) != 0) {
      break;
    }
  }
}

// DrawPixel draws the given color to the given x-y coordinates in the frame buffer. Returns nothing.
void Graphics::DrawPixel(int p_X, int p_Y, Color p_Color) {
  // Get pixel location based on pitch
  int s_Pixel = (p_Y * m_Width) + p_X;

  // Encode to 24-bit color
  uint32_t s_EncodedColor = 0x80000000 + (p_Color.a << 24) + (p_Color.r << 16) + (p_Color.g << 8) + p_Color.b;

  // Draw to the frame buffer
  if (p_Color.a == 255) {
    ((uint32_t *)m_FrameBuffers[m_ActiveFrameBufferIdx])[s_Pixel] = s_EncodedColor;
  } else {
    uint32_t *s_PixelPtr = &((uint32_t *)m_FrameBuffers[m_ActiveFrameBufferIdx])[s_Pixel];
    *s_PixelPtr = mix_color(s_PixelPtr, &s_EncodedColor); // Consumes a lot of CPU !
  }
}

// drawRectangle draws a rectangle at the given x-y xoordinates with the given width, height, and color to the frame
// buffer. Returns nothing.
void Graphics::DrawRectangle(int p_X, int p_Y, int p_Width, int p_Height, Color p_Color) {
  // Draw row-by-row, column-by-column
  for (int l_YPos = p_Y; l_YPos < p_Y + p_Height; l_YPos++) {
    for (int l_XPos = p_X; l_XPos < p_X + p_Width; l_XPos++) {
      DrawPixel(l_XPos, l_YPos, p_Color);
    }
  }
}

// initFont takes a vector of font paths and a font size, initializes the face pointer, and sets the font size. Returns error code.
int Graphics::InitTypefaces(std::vector<FT_Face> &p_Faces, std::vector<std::string> p_TypefacePaths, int p_FontSize) {
  p_Faces.clear();

  for (const std::string &l_TypefacePath : p_TypefacePaths) {
    FT_Face s_TempFace;

    if (FT_New_Face(m_FtLib, l_TypefacePath.c_str(), 0, &s_TempFace) != 0) {
      logKernel(LL_Debug, "Unable to load typeface: %s", l_TypefacePath.c_str());
      continue;
    }

    if (FT_Set_Pixel_Sizes(s_TempFace, 0, p_FontSize) != 0) {
      logKernel(LL_Debug, "Unable to set pixel size of typeface: %s", l_TypefacePath.c_str());
      continue;
    }
    p_Faces.push_back(s_TempFace);
  }

  if (p_Faces.size() == 0) {
    return -1;
  }

  return p_Faces.size();
}

int Graphics::SetFontSize(std::vector<FT_Face> &p_Faces, int p_FontSize) {
  int s_Ret = 0;

  for (size_t i = 0; i < p_Faces.size(); i++) {
    int s_TempRet = FT_Set_Pixel_Sizes(p_Faces[i], 0, p_FontSize);
    if (s_TempRet != 0) {
      logKernel(LL_Debug, "Failed to set font size for typeface at index %zu (Error #: %i)", i, s_TempRet);
      s_Ret += s_TempRet;
    }
  }

  return s_Ret;
}

// drawText uses the given face, starting coordinates, and background/foreground colors, and renders txt to the
// frame buffer. Returns nothing.
void Graphics::DrawText(const std::string &p_Text, std::vector<FT_Face> p_Faces, int p_StartX, int p_StartY, Color p_BgColor, Color p_FgColor) {
  int s_XOffset = 0;
  int s_YOffset = 0;

  size_t s_TypefaceIndex = 0;

  // Iterate each character of the text to write to the screen
  std::wstring s_WideText = Utility::StrToWstr(p_Text);
  for (size_t n = 0; n < s_WideText.size(); n++) {
    FT_UInt s_GlyphIndex;

    s_TypefaceIndex = 0;
    for (; s_TypefaceIndex < p_Faces.size(); s_TypefaceIndex++) {
      // Get the glyph for the uint16_t code
      s_GlyphIndex = FT_Get_Char_Index(p_Faces[s_TypefaceIndex], s_WideText[n]);

      if (s_GlyphIndex != 0 || s_WideText[n] == '\n') {
        break;
      }
    }

    // Get the glyph slot for bitmap and font metrics
    FT_GlyphSlot s_Slot = p_Faces[s_TypefaceIndex]->glyph;

    // Load and render in 8-bit color
    if (FT_Load_Glyph(p_Faces[s_TypefaceIndex], s_GlyphIndex, FT_LOAD_DEFAULT)) {
      continue;
    }

    if (FT_Render_Glyph(s_Slot, ft_render_mode_normal)) {
      continue;
    }

    // If we get a newline, increment the y offset, reset the x offset, and skip to the next character
    if (s_WideText[n] == '\n') {
      s_XOffset = 0;
      s_YOffset += s_Slot->bitmap.width * 2;

      continue;
    }

    // Parse and write the bitmap to the frame buffer
    for (int l_YPos = 0; l_YPos < s_Slot->bitmap.rows; l_YPos++) {
      for (int l_XPos = 0; l_XPos < s_Slot->bitmap.width; l_XPos++) {
        // Decode the 8-bit bitmap
        char pixel = s_Slot->bitmap.buffer[(l_YPos * s_Slot->bitmap.width) + l_XPos];

        // Get new pixel coordinates to account for the character position and baseline, as well as newlines
        int s_X = p_StartX + l_XPos + s_XOffset + s_Slot->bitmap_left;
        int s_Y = p_StartY + l_YPos + s_YOffset - s_Slot->bitmap_top;

        // Linearly interpolate between the foreground and background for smoother rendering
        uint8_t s_R = lerp(float(p_FgColor.r), float(p_BgColor.r), 255.0f);
        uint8_t s_G = lerp(float(p_FgColor.g), float(p_BgColor.g), 255.0f);
        uint8_t s_B = lerp(float(p_FgColor.b), float(p_BgColor.b), 255.0f);

        // Create new color struct with lerp'd values
        Color s_FinalColor = {s_R, s_G, s_B, 255};

        // We need to do bounds checking before commiting the pixel write due to our transformations, or we
        // could write out-of-bounds of the frame buffer
        if (s_X < 0 || s_Y < 0 || s_X >= m_Width || s_Y >= m_Height) {
          continue;
        }

        // If the pixel in the bitmap isn't blank, we'll draw it
        if (pixel != 0x00) {
          DrawPixel(s_X, s_Y, s_FinalColor);
        }
      }
    }

    // Increment x offset for the next character
    s_XOffset += s_Slot->advance.x >> 6;
  }
}

void Graphics::GetTextSize(const std::string &p_Text, std::vector<FT_Face> p_Faces, FontSize *p_Size) {
  int s_XOffset = 0;
  int s_YOffset = 0;

  size_t s_TypefaceIndex = 0;

  // Iterate each character of the text to write to the screen
  std::wstring s_WideText = Utility::StrToWstr(p_Text);
  for (size_t n = 0; n < s_WideText.size(); n++) {
    FT_UInt s_GlyphIndex;

    s_TypefaceIndex = 0;
    for (; s_TypefaceIndex < p_Faces.size(); s_TypefaceIndex++) {
      // Get the glyph for the uint16_t code
      s_GlyphIndex = FT_Get_Char_Index(p_Faces[s_TypefaceIndex], s_WideText[n]);

      if (s_GlyphIndex != 0 || s_WideText[n] == '\n') {
        break;
      }
    }

    // Get the glyph slot for bitmap and font metrics
    FT_GlyphSlot s_Slot = p_Faces[s_TypefaceIndex]->glyph;

    // Load and render in 8-bit color
    if (FT_Load_Glyph(p_Faces[0], s_GlyphIndex, FT_LOAD_DEFAULT)) {
      continue;
    }

    if (FT_Render_Glyph(s_Slot, ft_render_mode_normal)) {
      continue;
    }

    // If we get a newline, increment the y offset, reset the x offset, and skip to the next character
    if (s_WideText[n] == '\n') {
      s_XOffset = 0;
      s_YOffset += s_Slot->bitmap.width * 2;

      continue;
    }

    // Increment x offset for the next character
    s_XOffset += s_Slot->advance.x >> 6;
  }

  p_Size->width = s_XOffset;
  p_Size->height = s_YOffset - (p_Faces[s_TypefaceIndex]->glyph->metrics.horiBearingY >> 6);
}

void Graphics::DrawTextContainer(const std::string &p_Text, std::vector<FT_Face> p_Faces, int p_StartX, int p_StartY, int p_MaxWidth, int p_MaxHeight, Color p_BgColor, Color p_FgColor) {
  (void)p_MaxHeight; // UNUSED

  int s_XOffset = 0;
  int s_YOffset = 0;
  int s_CWidth = 0;
  int s_CHeight = 0;
  int s_Padding = 5;

  size_t s_TypefaceIndex = 0;

  // Iterate each character of the text to write to the screen
  std::wstring s_WideText = Utility::StrToWstr(p_Text);
  for (size_t n = 0; n < s_WideText.size(); n++) {
    FT_UInt s_GlyphIndex;

    s_TypefaceIndex = 0;
    for (; s_TypefaceIndex < p_Faces.size(); s_TypefaceIndex++) {
      // Get the glyph for the uint16_t code
      s_GlyphIndex = FT_Get_Char_Index(p_Faces[s_TypefaceIndex], s_WideText[n]);

      if (s_GlyphIndex != 0 || s_WideText[n] == '\n') {
        break;
      }
    }

    // Get the glyph slot for bitmap and font metrics
    FT_GlyphSlot s_Slot = p_Faces[s_TypefaceIndex]->glyph;

    // Load and render in 8-bit color
    if (FT_Load_Glyph(p_Faces[s_TypefaceIndex], s_GlyphIndex, FT_LOAD_DEFAULT)) {
      continue;
    }

    if (FT_Render_Glyph(s_Slot, ft_render_mode_normal)) {
      continue;
    }

    // If we get a newline, increment the y offset, reset the x offset, and skip to the next character
    if (s_WideText[n] == '\n') {
      s_XOffset = 0;
      s_YOffset += s_Slot->bitmap.width * 2;

      continue;
    }

    // word wrapping checks
    if ((s_CWidth + s_Slot->bitmap.width) >= p_MaxWidth) {
      s_CWidth = 0;
      s_CHeight += s_Slot->bitmap.width * 2;
      s_XOffset = 0;
      s_YOffset += (s_Slot->bitmap.width * 2) + s_Padding;
    } else {
      s_CWidth += s_Slot->bitmap.width;
    }

    // logKernel(LL_Debug, "%d - [WW] cWidth(%d) cHeight(%d) xOffset(%d) yOffset(%d) letter(%c)", n, cWidth, cHeight, xOffset, yOffset, txt[n]);
    // sceKernelUsleep(1000);

    // Parse and write the bitmap to the frame buffer
    for (int l_YPos = 0; l_YPos < s_Slot->bitmap.rows; l_YPos++) {
      for (int l_XPos = 0; l_XPos < s_Slot->bitmap.width; l_XPos++) {
        // Decode the 8-bit bitmap
        char s_Pixel = s_Slot->bitmap.buffer[(l_YPos * s_Slot->bitmap.width) + l_XPos];

        // Get new pixel coordinates to account for the character position and baseline, as well as newlines
        int s_X = p_StartX + l_XPos + s_XOffset + s_Slot->bitmap_left;
        int s_Y = p_StartY + l_YPos + s_YOffset - s_Slot->bitmap_top;

        // Linearly interpolate between the foreground and background for smoother rendering
        uint8_t s_R = lerp(float(p_FgColor.r), float(p_BgColor.r), 255.0f);
        uint8_t s_G = lerp(float(p_FgColor.g), float(p_BgColor.g), 255.0f);
        uint8_t s_B = lerp(float(p_FgColor.b), float(p_BgColor.b), 255.0f);

        // Create new color struct with lerp'd values
        Color s_FinalColor = {s_R, s_G, s_B, 255};

        // We need to do bounds checking before commiting the pixel write due to our transformations, or we
        // could write out-of-bounds of the frame buffer
        if (s_X < 0 || s_Y < 0 || s_X >= m_Width || s_Y >= m_Height) {
          continue;
        }

        // If the pixel in the bitmap isn't blank, we'll draw it
        if (s_Pixel != 0x00) {
          DrawPixel(s_X, s_Y, s_FinalColor);
        }
      }
    }

    // Increment x offset for the next character
    s_XOffset += s_Slot->advance.x >> 6;
  }
}

int Graphics::LoadPNG(Image *p_Image, const std::string &p_ImagePath) {
  logKernel(LL_Debug, "%s", "Loading PNG...");

  stbi_set_flip_vertically_on_load(0);
  stbi_set_flip_vertically_on_load_thread(0);
  p_Image->img = (uint32_t *)stbi_load(p_ImagePath.c_str(), &p_Image->width, &p_Image->height, &p_Image->channels, STBI_rgb_alpha);
  if (p_Image->img == NULL) {
    logKernel(LL_Debug, "Failed to load img: %s", stbi_failure_reason());
    return -1;
  }

  logKernel(LL_Debug, "%s", "PNG Loaded!");

  p_Image->use_alpha = true;

  return 0;
}

int Graphics::LoadPNGFromMemory(Image *p_Image, unsigned char *p_ImageData, int p_ImageSize) {
  logKernel(LL_Debug, "%s", "Loading PNG from memory...");

  stbi_set_flip_vertically_on_load(0);
  stbi_set_flip_vertically_on_load_thread(0);
  p_Image->img = (uint32_t *)stbi_load_from_memory((stbi_uc *)p_ImageData, p_ImageSize, &p_Image->width, &p_Image->height, &p_Image->channels, STBI_rgb_alpha);
  if (p_Image->img == NULL) {
    logKernel(LL_Debug, "Failed to load img: %s", stbi_failure_reason());
    return -1;
  }

  logKernel(LL_Debug, "%s", "PNG Loaded!");

  p_Image->use_alpha = false;
  return 0;
}

void Graphics::DrawPNG(Image *p_Image, int p_StartX, int p_StartY) {
  if (p_Image == NULL) {
    logKernel(LL_Debug, "%s", "Image is not initialized!");
    return;
  }

  // Iterate the bitmap and draw the pixels
  for (int l_YPos = 0; l_YPos < p_Image->height; l_YPos++) {
    for (int l_XPos = 0; l_XPos < p_Image->width; l_XPos++) {
      // Decode the 32-bit color
      uint32_t s_EncodedColor = p_Image->img[(l_YPos * p_Image->width) + l_XPos];

      // Get new pixel coordinates to account for start coordinates
      int s_X = p_StartX + l_XPos;
      int s_Y = p_StartY + l_YPos;

      // Re-encode the color
      uint8_t s_R = s_EncodedColor;
      uint8_t s_G = s_EncodedColor >> 8;
      uint8_t s_B = s_EncodedColor >> 16;
      uint8_t s_A = s_EncodedColor >> 24;

      if (!p_Image->use_alpha) {
        s_A = 0xFF;
      }

      Color s_Color = {s_R, s_G, s_B, s_A};

      // Do some bounds checking to make sure the pixel is actually inside the frame buffer
      if (l_XPos < 0 || l_YPos < 0 || l_XPos >= p_Image->width || l_YPos >= p_Image->height) {
        continue;
      }

      DrawPixel(s_X, s_Y, s_Color);
    }
  }
}

void Graphics::DrawSizedPNG(Image *p_Image, int p_StartX, int p_StartY, int p_Width, int p_Height) {
  if (p_Image == NULL) {
    logKernel(LL_Debug, "%s", "Image is not initialized!");
    return;
  }

  int s_XRatio = 1;
  int s_YRatio = 1;

  if (p_Width <= 0 || p_Height <= 0) {
    logKernel(LL_Debug, "%s", "Trying to divide by 0!");
  } else {
    s_XRatio = (int)((p_Image->width << 16) / p_Width);   // +1
    s_YRatio = (int)((p_Image->height << 16) / p_Height); // +1
  }

  int s_X2;
  int s_Y2;

  // Iterate the bitmap and draw the pixels
  for (int l_YPos = 0; l_YPos < p_Height; l_YPos++) {
    for (int l_XPos = 0; l_XPos < p_Width; l_XPos++) {
      // Calculate
      s_X2 = ((l_XPos * s_XRatio) >> 16);
      s_Y2 = ((l_YPos * s_YRatio) >> 16);

      // Decode the 32-bit color
      uint32_t s_EncodedColor = p_Image->img[(s_Y2 * p_Image->width) + s_X2];

      // Get new pixel coordinates to account for start coordinates
      int s_X = p_StartX + l_XPos;
      int s_Y = p_StartY + l_YPos;

      // Re-encode the color
      uint8_t s_R = (uint8_t)(s_EncodedColor >> 0);
      uint8_t s_G = (uint8_t)(s_EncodedColor >> 8);
      uint8_t s_B = (uint8_t)(s_EncodedColor >> 16);
      uint8_t s_A = (uint8_t)(s_EncodedColor >> 24);

      if (!p_Image->use_alpha) {
        s_A = 0xFF;
      }

      Color s_Color = {s_R, s_G, s_B, s_A};
      DrawPixel(s_X, s_Y, s_Color);
    }
  }
}

void Graphics::UnloadPNG(Image *p_Image) {
  if (p_Image->img) {
    stbi_image_free(p_Image->img);
  }

  p_Image->img = NULL;
  p_Image->width = 0;
  p_Image->height = 0;
  p_Image->channels = 0;
}

int Graphics::GetScreenWidth() {
  return m_Width;
}

int Graphics::GetScreenHeight() {
  return m_Height;
}

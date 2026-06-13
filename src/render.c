#include "types.h"

EFI_GRAPHICS_OUTPUT_PROTOCOL  *gGOP          = NULL;
UINT32                         gScreenWidth  = 0;
UINT32                         gScreenHeight = 0;
Pixel                         *gBackBuffer   = NULL;
Pixel                         *gGameBuffer   = NULL;
Pixel                         *gScaledBuffer = NULL;
UINT32                         gScale        = 1;
INT32                          gOffsetX      = 0;
INT32                          gOffsetY      = 0;

EFI_STATUS GraphicsInit(VOID)
{
  EFI_STATUS  Status;
  UINTN       size;

  Status = gBS->LocateProtocol(
    &gEfiGraphicsOutputProtocolGuid,
    NULL,
    (VOID **)&gGOP
  );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "[Graphics] Cannot locate GOP: %r\n", Status));
    return Status;
  }

  gScreenWidth  = gGOP->Mode->Info->HorizontalResolution;
  gScreenHeight = gGOP->Mode->Info->VerticalResolution;

  DEBUG((DEBUG_INFO, "[Graphics] Display: %dx%d\n", gScreenWidth, gScreenHeight));

  // Determine scale factor: largest integer that fits
  gScale = 1;
  for (UINT32 s = MAX_SCALE; s >= 1; s--) {
    if (GAME_WIDTH * s <= gScreenWidth && GAME_HEIGHT * s <= gScreenHeight) {
      gScale = s;
      break;
    }
  }

  // Center the game area
  gOffsetX = ((INT32)gScreenWidth  - (INT32)(GAME_WIDTH  * gScale)) / 2;
  gOffsetY = ((INT32)gScreenHeight - (INT32)(GAME_HEIGHT * gScale)) / 2;

  DEBUG((DEBUG_INFO, "[Graphics] Scale: %dx, Offset: %d,%d\n", gScale, gOffsetX, gOffsetY));

  // Allocate full-screen back buffer
  size = gScreenWidth * gScreenHeight * sizeof(Pixel);
  gBackBuffer = (Pixel *)AllocateZeroPool(size);
  if (gBackBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Allocate 256x240 game buffer
  size = GAME_WIDTH * GAME_HEIGHT * sizeof(Pixel);
  gGameBuffer = (Pixel *)AllocateZeroPool(size);
  if (gGameBuffer == NULL) {
    FreePool(gBackBuffer);
    gBackBuffer = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  // Allocate scaled buffer
  UINT32 scaledW = GAME_WIDTH  * gScale;
  UINT32 scaledH = GAME_HEIGHT * gScale;
  size = scaledW * scaledH * sizeof(Pixel);
  gScaledBuffer = (Pixel *)AllocateZeroPool(size);
  if (gScaledBuffer == NULL) {
    FreePool(gGameBuffer);
    FreePool(gBackBuffer);
    gGameBuffer = NULL;
    gBackBuffer = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

VOID GraphicsShutdown(VOID)
{
  if (gScaledBuffer != NULL) { FreePool(gScaledBuffer); gScaledBuffer = NULL; }
  if (gGameBuffer   != NULL) { FreePool(gGameBuffer);   gGameBuffer   = NULL; }
  if (gBackBuffer   != NULL) { FreePool(gBackBuffer);   gBackBuffer   = NULL; }
  gGOP = NULL;
}

VOID ClearGameBuffer(Pixel color)
{
  UINT32 i;
  UINT32 count = GAME_WIDTH * GAME_HEIGHT;
  for (i = 0; i < count; i++) {
    gGameBuffer[i] = color;
  }
}

VOID PutPixel(UINT32 x, UINT32 y, Pixel color)
{
  if (x < GAME_WIDTH && y < GAME_HEIGHT) {
    gGameBuffer[y * GAME_WIDTH + x] = color;
  }
}

VOID FillRect(INT32 x, INT32 y, UINT32 w, UINT32 h, Pixel color)
{
  UINT32 row, col;
  INT32  x0 = x, y0 = y;

  // Reject completely off-screen
  if (x0 >= (INT32)GAME_WIDTH || y0 >= (INT32)GAME_HEIGHT) return;
  if (x0 + (INT32)w <= 0 || y0 + (INT32)h <= 0) return;

  // Clip left/top
  if (x0 < 0) { if ((UINT32)(-x0) >= w) return; w -= (UINT32)(-x0); x0 = 0; }
  if (y0 < 0) { if ((UINT32)(-y0) >= h) return; h -= (UINT32)(-y0); y0 = 0; }

  // Clip right/bottom
  if ((UINT32)x0 >= GAME_WIDTH || (UINT32)y0 >= GAME_HEIGHT) return;
  if ((UINT32)x0 + w > GAME_WIDTH)  w = GAME_WIDTH  - (UINT32)x0;
  if ((UINT32)y0 + h > GAME_HEIGHT) h = GAME_HEIGHT - (UINT32)y0;
  if (w == 0 || h == 0) return;

  for (row = 0; row < h; row++) {
    Pixel *rowPtr = &gGameBuffer[(y0 + row) * GAME_WIDTH + x0];
    for (col = 0; col < w; col++) {
      rowPtr[col] = color;
    }
  }
}

VOID ScaleAndPresent(VOID)
{
  UINT32 scaledW = GAME_WIDTH  * gScale;
  UINT32 scaledH = GAME_HEIGHT * gScale;

  // Integer scale with contrast boost
  for (UINT32 gy = 0; gy < GAME_HEIGHT; gy++) {
    Pixel *srcRow = &gGameBuffer[gy * GAME_WIDTH];
    for (UINT32 gx = 0; gx < GAME_WIDTH; gx++) {
      Pixel px = srcRow[gx];
      // Fill gScale×gScale block
      for (UINT32 dy = 0; dy < gScale; dy++) {
        UINT32 sy = gy * gScale + dy;
        Pixel *dstRow = &gScaledBuffer[sy * scaledW + gx * gScale];
        for (UINT32 dx = 0; dx < gScale; dx++) {
          dstRow[dx] = px;
        }
      }
    }
  }

  // Clear back buffer
  UINTN totalPixels = gScreenWidth * gScreenHeight;
  for (UINTN i = 0; i < totalPixels; i++) {
    gBackBuffer[i] = COLOR_BLACK;
  }

  // Copy scaled buffer to back buffer centered
  for (UINT32 sy = 0; sy < scaledH; sy++) {
    Pixel *dstRow = &gBackBuffer[((UINT32)gOffsetY + sy) * gScreenWidth + (UINT32)gOffsetX];
    Pixel *srcRow = &gScaledBuffer[sy * scaledW];
    CopyMem(dstRow, srcRow, scaledW * sizeof(Pixel));
  }

  // Blt to screen
  gGOP->Blt(
    gGOP,
    (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)gBackBuffer,
    EfiBltBufferToVideo,
    0, 0, 0, 0,
    gScreenWidth, gScreenHeight,
    gScreenWidth * sizeof(Pixel)
  );
}

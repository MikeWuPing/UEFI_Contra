#include "sprites.h"
#include <Protocol/SimpleFileSystem.h>

SPRITE_FRAME gSpriteFrames[MAX_SPRITES];

static EFI_FILE_PROTOCOL *gRootFS = NULL;

// Load a .spr file by its full filename into a specific sprite code
static VOID LoadNamedSprite(UINT32 code, CHAR16 *filename)
{
  EFI_FILE_PROTOCOL *sprFile;
  EFI_STATUS         Status;
  UINTN              headerSize;
  UINT8              header[4];

  Status = gRootFS->Open(gRootFS, &sprFile, filename, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(Status)) return;

  headerSize = 4;
  Status = sprFile->Read(sprFile, &headerSize, header);
  if (EFI_ERROR(Status) || headerSize != 4) {
    sprFile->Close(sprFile);
    return;
  }

  UINT16 w = header[0] | (header[1] << 8);
  UINT16 h = header[2] | (header[3] << 8);
  if (w == 0 || h == 0 || w > 256 || h > 256) {
    sprFile->Close(sprFile);
    return;
  }

  UINTN pixelSize = (UINTN)w * h * sizeof(UINT32);
  gSpriteFrames[code].Pixels = AllocatePool(pixelSize);
  if (gSpriteFrames[code].Pixels == NULL) {
    sprFile->Close(sprFile);
    return;
  }

  gSpriteFrames[code].Width  = w;
  gSpriteFrames[code].Height = h;

  UINTN bytesRead = pixelSize;
  Status = sprFile->Read(sprFile, &bytesRead, gSpriteFrames[code].Pixels);
  sprFile->Close(sprFile);

  if (EFI_ERROR(Status)) {
    FreePool(gSpriteFrames[code].Pixels);
    gSpriteFrames[code].Pixels = NULL;
  }
}

// Try to load a single .spr file by sprite code
static VOID LoadOneSprite(UINT32 code)
{
  EFI_FILE_PROTOCOL *sprFile;
  EFI_STATUS         Status;
  CHAR16             path[32];
  UINTN              headerSize;
  UINT8              header[4];

  UnicodeSPrint(path, sizeof(path), L"sprites\\sprite_%02x.spr", code);

  Status = gRootFS->Open(gRootFS, &sprFile, path, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(Status)) return;

  // Read 4-byte header: width (UINT16 LE), height (UINT16 LE)
  headerSize = 4;
  Status = sprFile->Read(sprFile, &headerSize, header);
  if (EFI_ERROR(Status) || headerSize != 4) {
    sprFile->Close(sprFile);
    return;
  }

  UINT16 w = header[0] | (header[1] << 8);
  UINT16 h = header[2] | (header[3] << 8);

  if (w == 0 || h == 0 || w > 256 || h > 256) {
    sprFile->Close(sprFile);
    return;
  }

  UINTN pixelSize = (UINTN)w * h * sizeof(UINT32);
  gSpriteFrames[code].Pixels = AllocatePool(pixelSize);
  if (gSpriteFrames[code].Pixels == NULL) {
    sprFile->Close(sprFile);
    return;
  }

  gSpriteFrames[code].Width  = w;
  gSpriteFrames[code].Height = h;

  UINTN bytesRead = pixelSize;
  Status = sprFile->Read(sprFile, &bytesRead, gSpriteFrames[code].Pixels);
  sprFile->Close(sprFile);

  if (EFI_ERROR(Status)) {
    FreePool(gSpriteFrames[code].Pixels);
    gSpriteFrames[code].Pixels = NULL;
  }
}

// Also try _p2 variant for player 2 sprites
static VOID LoadOneSpriteP2(UINT32 code)
{
  EFI_FILE_PROTOCOL *sprFile;
  EFI_STATUS         Status;
  CHAR16             path[40];
  UINTN              headerSize;
  UINT8              header[4];
  UINT32             p2code;

  p2code = code | 0x80; // Use high bit to distinguish P2 variants
  if (p2code >= MAX_SPRITES) return;

  UnicodeSPrint(path, sizeof(path), L"sprites\\sprite_%02x_p2.spr", code);

  Status = gRootFS->Open(gRootFS, &sprFile, path, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(Status)) return;

  headerSize = 4;
  Status = sprFile->Read(sprFile, &headerSize, header);
  if (EFI_ERROR(Status) || headerSize != 4) {
    sprFile->Close(sprFile);
    return;
  }

  UINT16 w = header[0] | (header[1] << 8);
  UINT16 h = header[2] | (header[3] << 8);

  if (w == 0 || h == 0 || w > 256 || h > 256) {
    sprFile->Close(sprFile);
    return;
  }

  UINTN pixelSize = (UINTN)w * h * sizeof(UINT32);
  gSpriteFrames[p2code].Pixels = AllocatePool(pixelSize);
  if (gSpriteFrames[p2code].Pixels == NULL) {
    sprFile->Close(sprFile);
    return;
  }

  gSpriteFrames[p2code].Width  = w;
  gSpriteFrames[p2code].Height = h;

  UINTN bytesRead = pixelSize;
  Status = sprFile->Read(sprFile, &bytesRead, gSpriteFrames[p2code].Pixels);
  sprFile->Close(sprFile);

  if (EFI_ERROR(Status)) {
    FreePool(gSpriteFrames[p2code].Pixels);
    gSpriteFrames[p2code].Pixels = NULL;
  }
}

EFI_STATUS SpriteSystemInit(VOID)
{
  EFI_STATUS                      Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFS;
  UINT32                          i;
  UINTN                           fileCount = 0;

  ZeroMem(gSpriteFrames, sizeof(gSpriteFrames));

  // Locate the filesystem
  Status = gBS->LocateProtocol(
    &gEfiSimpleFileSystemProtocolGuid,
    NULL,
    (VOID **)&SimpleFS
  );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "[Sprites] No FS protocol: %r\n", Status));
    return Status;
  }

  // Open root directory
  Status = SimpleFS->OpenVolume(SimpleFS, &gRootFS);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "[Sprites] Cannot open root: %r\n", Status));
    return Status;
  }

  // Iterate over all possible sprite codes (0x02 to 0xCF)
  for (i = 2; i <= 0xCF; i++) {
    LoadOneSprite(i);
    if (gSpriteFrames[i].Pixels != NULL) fileCount++;

    // Try _p2 variant for player-related sprites (0x02-0x1f, 0x50-0x58, 0x91)
    if (i <= 0x58) {
      LoadOneSpriteP2(i);
      UINT32 p2code = i | 0x80;
      if (p2code < MAX_SPRITES && gSpriteFrames[p2code].Pixels != NULL) fileCount++;
    }
  }

  DEBUG((DEBUG_INFO, "[Sprites] Loaded %d sprite frames\n", fileCount));

  // Load medal sprites for lives display (they use named files, not hex codes)
  LoadNamedSprite(0xE0, L"sprites\\player_1_lives_medal.spr");
  LoadNamedSprite(0xE1, L"sprites\\player_2_lives_medal.spr");

  return EFI_SUCCESS;
}

VOID SpriteSystemShutdown(VOID)
{
  UINT32 i;
  for (i = 0; i < MAX_SPRITES; i++) {
    if (gSpriteFrames[i].Pixels != NULL) {
      FreePool(gSpriteFrames[i].Pixels);
      gSpriteFrames[i].Pixels = NULL;
    }
  }
  if (gRootFS != NULL) {
    gRootFS->Close(gRootFS);
    gRootFS = NULL;
  }
}

VOID DrawSprite(
  UINT32 spriteCode,
  INT32  x,
  INT32  y,
  UINT8  flipFlags,
  UINT32 colorMask)
{
  SPRITE_FRAME *frame;
  INT32 row, col;
  INT32 srcX, srcY;
  UINT32 pixel;

  if (spriteCode >= MAX_SPRITES) return;
  frame = &gSpriteFrames[spriteCode];
  if (frame->Pixels == NULL || frame->Width == 0 || frame->Height == 0) return;

  // Draw 4-direction dark outline for sprite/background separation
  for (row = 0; row < (INT32)frame->Height; row++) {
    for (col = 0; col < (INT32)frame->Width; col++) {
      if (flipFlags & 0x01) srcX = (INT32)frame->Width - 1 - col;
      else srcX = col;
      if (flipFlags & 0x02) srcY = (INT32)frame->Height - 1 - row;
      else srcY = row;

      pixel = frame->Pixels[srcY * frame->Width + srcX];
      if (pixel == 0x00000000) continue;

      // 4-directional dark outline
      static const INT32 ox[4] = {1, -1, 0, 0};
      static const INT32 oy[4] = {0, 0, 1, -1};
      for (INT32 d = 0; d < 4; d++) {
        INT32 dx = x + col + ox[d];
        INT32 dy = y + row + oy[d];
        if (dx >= 0 && dx < GAME_WIDTH && dy >= 0 && dy < GAME_HEIGHT) {
          Pixel *dst = &gGameBuffer[dy * GAME_WIDTH + dx];
          // Darken by 75% (strong outline)
          UINT32 r = ((*dst >> 16) & 0xFF) >> 2;
          UINT32 g = ((*dst >> 8) & 0xFF) >> 2;
          UINT32 b = (*dst & 0xFF) >> 2;
          *dst = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
      }
    }
  }

  // Draw actual sprite
  for (row = 0; row < (INT32)frame->Height; row++) {
    for (col = 0; col < (INT32)frame->Width; col++) {
      if (flipFlags & 0x01) srcX = (INT32)frame->Width - 1 - col;
      else srcX = col;

      if (flipFlags & 0x02) srcY = (INT32)frame->Height - 1 - row;
      else srcY = row;

      pixel = frame->Pixels[srcY * frame->Width + srcX];

      if (pixel == 0x00000000) continue;

      pixel &= colorMask;

      INT32 dx = x + col;
      INT32 dy = y + row;

      if (dx >= 0 && dx < GAME_WIDTH && dy >= 0 && dy < GAME_HEIGHT) {
        gGameBuffer[dy * GAME_WIDTH + dx] = pixel;
      }
    }
  }
}

#include "nes_palette.h"

// NES 64-color master palette - standard NTSC NES PPU palette values
// Index 0x0D, 0x0E, 0x0F, 0x1D, 0x1E, 0x1F, 0x2D, 0x2E, 0x2F,
//        0x3D, 0x3E, 0x3F are all black/dark shades
const UINT32 gNesPaletteBGR[NES_PALETTE_SIZE] = {
  // 0x00-0x0F (0xFF prefix = opaque alpha)
  0xFF7C7C7C, // $00 Gray
  0xFF0000FC, // $01 Blue
  0xFF0000BC, // $02 Dark Blue
  0xFF4428BC, // $03 Purple
  0xFF940084, // $04 Magenta
  0xFFA80020, // $05 Dark Red
  0xFFA81000, // $06 Red
  0xFF881400, // $07 Brown
  0xFF503000, // $08 Olive
  0xFF007800, // $09 Dark Green
  0xFF006800, // $0A Green
  0xFF005858, // $0B Teal
  0xFF00548C, // $0C Cyan
  0xFF000000, // $0D Black
  0xFF000000, // $0E Black
  0xFF000000, // $0F Black
  // 0x10-0x1F
  0xFFBCBCBC, // $10 Light Gray
  0xFF0070FC, // $11 Light Blue
  0xFF4438FC, // $12 Periwinkle
  0xFF803CEC, // $13 Violet
  0xFFC858C4, // $14 Pink
  0xFFE83874, // $15 Salmon
  0xFFF83800, // $16 Orange
  0xFFD87800, // $17 Gold
  0xFFA89000, // $18 Golden Olive (muted for stone wall accuracy)
  0xFF788820, // $19 Earthy Olive (further muted for stone walls)
  0xFF38A818, // $1A Sea Green
  0xFF38A068, // $1B Blue-green
  0xFF3878A0, // $1C Sky Blue
  0xFF000000, // $1D Black
  0xFF000000, // $1E Black
  0xFF000000, // $1F Black
  // 0x20-0x2F
  0xFFFCFCFC, // $20 White
  0xFF3CA0FC, // $21 Bright Blue
  0xFF8878FC, // $22 Lavender
  0xFFB878F8, // $23 Light Purple
  0xFFF878F8, // $24 Hot Pink
  0xFFF8A0C0, // $25 Light Pink
  0xFFF8C878, // $26 Peach
  0xFFF8E070, // $27 Light Yellow
  0xFFC08020, // $28 Warm Amber (adjusted for stone/concrete tones) → adjusted warmer/amber for NTSC accuracy
  0xFFC0C890, // $29 Muted Khaki (further desaturated for stone walls)
  0xFFB8F8D8, // $2A Aqua
  0xFFB8F0F8, // $2B Light Cyan
  0xFFC8E0F8, // $2C Pale Blue
  0xFF000000, // $2D Black
  0xFF000000, // $2E Black
  0xFF000000, // $2F Black
  // 0x30-0x3F
  0xFFFCFCFC, // $30 Pure White
  0xFFA8D0FC, // $31 Sky
  0xFFC4C8FC, // $32 Light Periwinkle
  0xFFD0C4F8, // $33
  0xFFF8C0F8, // $34
  0xFFF8D0E0, // $35
  0xFFFCE0C0, // $36
  0xFFFCF0C0, // $37
  0xFFE8F8C8, // $38
  0xFFD0F8D8, // $39
  0xFFD0F8F0, // $3A
  0xFFD8F0FC, // $3B
  0xFFE4E8F8, // $3C
  0xFF000000, // $3D Black
  0xFF000000, // $3E Black
  0xFF000000, // $3F Black
};

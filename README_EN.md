# Contra UEFI

> 🎮 Run the classic NES Contra on UEFI firmware — a bare-metal game porting experiment

**中文版：[README.md](README.md)**

---

## Overview

This project ports the classic NES game **Contra** (魂斗罗) to the **UEFI Shell** environment. The game runs as a UEFI application (.efi file), rendering graphics directly through GOP (Graphics Output Protocol) without requiring an operating system — only UEFI firmware.

The implementation references the 6502 assembly source from the [NES Contra US Disassembly](https://github.com/vermiceli/nes-contra-us) project, reconstructing the core game logic in C. Sprite assets are derived from graphics extracted from the original NES ROM.

### Features

- 🖥️ **Bare-metal execution** — Boots directly on QEMU + OVMF firmware, no OS required
- 🎨 **GOP graphics** — Double-buffered 256×240 game canvas, integer-scaled to display resolution
- ⌨️ **Keyboard input** — Arrow keys to move, Z to fire, X to jump, Enter to start
- 🧩 **Authentic NES architecture** — Full super-tile background system, 2bpp tile + palette rendering, RLE decompression
- 🔫 **6 weapon types** — Default / Machine Gun / Fire Ball / Spread / Laser / Barrier
- 👾 **Enemy AI** — Soldier, Turret, Boss Gun, and Runner enemy types
- 🗺️ **Level data port** — 13-screen horizontal scrolling level with 103 super-tiles
- 💾 **Filesystem-based sprites** — Loads .spr sprite files from FAT disk via UEFI Simple File System Protocol

---

---

## Architecture

### Per-Frame Rendering Pipeline (60fps)

```
PollInput()
  └─ GameStateMachine[gGameRoutineIndex]()
       └─ LevelRoutine04 (core gameplay)
            ├─ UpdatePlayer()
            ├─ UpdatePlayerBullets()
            ├─ UpdateEnemies()
            ├─ SpawnScreenEnemies()
            └─ UpdateScroll()

RenderGame()
  ├─ ClearGameBuffer(BLACK)
  ├─ DrawBackground()          # Super-Tile → 8×8 NES 2bpp tile composition
  ├─ DrawWeaponItems()
  ├─ DrawPlayerBullets()
  ├─ DrawEnemyBullets()
  ├─ DrawEnemies()
  ├─ DrawPlayer()
  ├─ DrawHUD()                 # Lives, score, weapon indicator
  └─ ScaleAndPresent()         # Integer scaling + GOP Blt
```

### NES Graphics Three-Layer Hierarchy

| Layer | Unit | Size | Description |
|-------|------|------|-------------|
| Pattern Table | Tile | 8×8 px | 2bpp format (16 bytes/tile), 4-color palette |
| Super-Tile | 4×4 Tiles | 32×32 px | 16-byte index, per-quadrant palette assignment |
| Screen | 8×7 Super-Tiles | 256×224 px | RLE-compressed, 56 bytes expanded |

### Game State Machine

```
Game Routines (7 states)
  00 → 01 → 02 → 03 → 04 → 05 ←→ 06
  Logo  Select Demo  Flash Clear Core  Ending

Level Routines (11 sub-states, within Game Routine 05)
  00 → 01 → 02 → 03 → 04 ←→ 05 → 06/07 → 08 → 09 → 0a
  Load  Show  Flash Draw  Core  Clear  GAMEOVER BossDie Seq  Delay
```

### Collision Detection

Four collision codes based on tile index thresholds:
- **Code 0** — Empty (free passage)
- **Code 1** — Floor (standable, wall-jump-down)
- **Code 2** — Water (no jumping, lower body hidden)
- **Code 3** — Solid (fully impassable)

---

## Quick Start

### Prerequisites

- **Windows 10/11** + **Visual Studio 2019**
- **NASM** — Netwide Assembler
- **QEMU** — for running the test environment
- **Python 3.x** — for data extraction and sprite conversion

### Build

```bash

# 2. Convert sprite images (PNG → .spr)
cd tools
python png_to_spr.py
cd ..

# 3. Build the UEFI application
cd ../edk2
export WORKSPACE="$(pwd)"
export EDK_TOOLS_PATH="$WORKSPACE/BaseTools"
export NASM_PREFIX="/c/Program Files/NASM/"
export VS2019_PREFIX="/c/Program Files (x86)/Microsoft Visual Studio/2019/Professional/VC/Tools/MSVC/14.29.30133/"
export WINSDK10_PREFIX="/c/Program Files (x86)/Windows Kits/10/"
export PATH="$NASM_PREFIX:$WORKSPACE/BaseTools/Bin/Win64:$VS2019_PREFIX/bin/Hostx64/x64:$PATH"
export PYTHONUTF8=1
export PYTHONPATH="$WORKSPACE/BaseTools/Source/Python"

python BaseTools/Source/Python/build/build.py \
  -p EmulatorPkg/EmulatorPkg.dsc \
  -m EmulatorPkg/Application/ContraGame/ContraGame.inf \
  -a X64 -t VS2019 -b DEBUG

# 4. Deploy and test
cp Build/EmulatorX64/DEBUG_VS2019/X64/Contra.efi ../contra/qemu_test/
cd ../contra/qemu_test

qemu-system-x86_64 \
  -drive if=pflash,format=raw,file=OVMF_CODE.fd,readonly=on \
  -drive file=fat:rw:.,format=raw \
  -net none -m 256 -vga std
```

Type `Contra` at the UEFI Shell prompt to start.

---

## Controls

| Key | Action |
|-----|--------|
| ← → | Move horizontally |
| ↑ ↓ | Aim direction / Menu selection |
| Z | Fire |
| X | Jump |
| Enter | Start / Confirm |
| P | Pause |
| ESC | Exit game |

---

## Design Decisions

This project aims to faithfully reproduce the core Contra experience in UEFI while making pragmatic trade-offs:

- **No audio** — Audio driver complexity in UEFI is high, and QEMU emulation is limited
- **Static allocation** — All game arrays are compile-time fixed-size, avoiding runtime fragmentation and allocation failures
- **Integer scaling** — 256×240 canvas integer-scaled (2×/3×) to modern resolutions, preventing fractional-scaling artifacts
- **Pre-converted sprites** — PNG images are converted to `.spr` binary format before compilation, loaded at runtime via UEFI filesystem protocol to keep .efi size manageable
- **Fixed-point physics** — INT32 8.8 fixed-point format eliminates floating-point operations, ensuring cross-platform precision

---

## Code Stats

| Category | Lines |
|----------|-------|
| C source (.c) | ~3,200 |
| Headers (.h) | ~1,900 |
| Level data | ~650 |
| Python tools | ~1,800 |
| **Total** | **~7,500** |

---

## References

- [NES Contra US Disassembly](https://github.com/vermiceli/nes-contra-us) — Complete 6502 reverse-engineering project
- [Retro Game Internals: Contra Levels](https://tomorrowcorporation.com/posts/retro-game-internals-contra-levels) — Allan Blomquist's deep dive into Contra's level structure
- [NESDev Wiki](https://www.nesdev.org/wiki/) — NES PPU, palette, and nametable technical specifications

---

## License

The source code in this project is released under the **MIT License**.

The NES disassembly reference and sprite assets originate from the [nes-contra-us](https://github.com/vermiceli/nes-contra-us) project. All rights to the original Contra game assets belong to their respective rights holders.

---

## Acknowledgments

- [vermiceli](https://github.com/vermiceli) and contributors — The comprehensive NES Contra disassembly that made this port possible
- UEFI / TianoCore community — EDK2 development framework and OVMF firmware
- The QEMU project — Powerful open-source emulator enabling development and testing

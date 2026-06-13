# Contra UEFI

> 🎮 在 UEFI 固件环境中运行经典 NES 魂斗罗 — 一个裸金属级的游戏移植实验

**English version: [README_EN.md](README_EN.md)**

---

## 项目概述

这是一个将红白机经典《魂斗罗》（Contra）移植到 **UEFI Shell** 环境的技术实验项目。游戏作为 UEFI 应用程序（.efi 文件）运行，直接通过 GOP（Graphics Output Protocol）渲染画面，无需操作系统支持，仅依赖 UEFI 固件。

项目参考了 [NES Contra US Disassembly](https://github.com/vermiceli/nes-contra-us) 的 6502 汇编源码，在 C 语言层面重建了核心游戏逻辑，精灵图片来源于 NES ROM 中提取的素材。

<img width="771" height="663" alt="short1" src="https://github.com/user-attachments/assets/31a859b3-ea3a-4bc4-9812-d8b355e0077c" />
<img width="482" height="432" alt="Snipaste_2026-06-13_08-12-50" src="https://github.com/user-attachments/assets/99912672-87cb-4e38-9126-ed755783f347" />
<img width="771" height="663" alt="Snipaste_2026-06-13_19-18-18" src="https://github.com/user-attachments/assets/908d8acd-456d-4809-9454-8cf2014b1d53" />


### 特性

- 🖥️ **裸金属运行** — 在 QEMU + OVMF 固件上直接启动，无操作系统
- 🎨 **GOP 图形渲染** — 双缓冲 256×240 游戏画面，整数缩放至显示分辨率
- ⌨️ **键盘输入** — 方向键移动、Z 射击、X 跳跃、Enter 开始
- 🧩 **NES 架构还原** — 完整移植 super-tile 背景系统、2bpp tile + 调色板渲染、RLE 解压
- 🔫 **6 种武器** — Default / Machine Gun / Fire Ball / Spread / Laser / Barrier
- 👾 **敌人 AI** — 士兵、炮塔、Boss 炮台、奔跑者 4 种类型
- 🗺️ **关卡数据** — 13 个屏幕的横向卷轴关卡，含 103 个 super-tile
- 💾 **文件系统加载精灵** — 通过 UEFI Simple File System Protocol 从 FAT 磁盘读取 .spr 精灵文件

---

## 运行截图

> 在 QEMU 中启动 UEFI Shell，输入 `Contra` 即可开始游戏。

<img width="482" height="432" alt="Snipaste_2026-06-13_08-12-50" src="https://github.com/user-attachments/assets/99912672-87cb-4e38-9126-ed755783f347" />

---

---

## 技术架构

### 渲染管线（每帧 60fps）

```
PollInput()
  └─ GameStateMachine[gGameRoutineIndex]()
       └─ LevelRoutine04（核心游戏逻辑）
            ├─ UpdatePlayer()
            ├─ UpdatePlayerBullets()
            ├─ UpdateEnemies()
            ├─ SpawnScreenEnemies()
            └─ UpdateScroll()

RenderGame()
  ├─ ClearGameBuffer(BLACK)
  ├─ DrawBackground()          # Super-Tile → 8×8 NES 2bpp tile 合成
  ├─ DrawWeaponItems()
  ├─ DrawPlayerBullets()
  ├─ DrawEnemyBullets()
  ├─ DrawEnemies()
  ├─ DrawPlayer()
  ├─ DrawHUD()                 # 生命数、分数、武器指示
  └─ ScaleAndPresent()         # 整数缩放 + GOP Blt
```

### NES 图形三层架构

| 层级 | 单位 | 尺寸 | 说明 |
|------|------|------|------|
| Pattern Table | Tile | 8×8 像素 | 2bpp 格式（16 字节/tile），4 色调色板 |
| Super-Tile | 4×4 Tiles | 32×32 像素 | 16 字节索引，4 象限独立调色板 |
| Screen | 8×7 Super-Tiles | 256×224 像素 | RLE 压缩存储，展开后 56 字节 |

### 游戏状态机

```
Game Routines (7 状态)
  00 → 01 → 02 → 03 → 04 → 05 ←→ 06
  Logo  选择  Demo  闪屏 清空  核心  结局

Level Routines (11 子状态, 在 Game Routine 05 内)
  00 → 01 → 02 → 03 → 04 ←→ 05 → 06/07 → 08 → 09 → 0a
 加载  显示 闪烁  绘制  核心  过关  GAMEOVER Boss死 序列  延迟
```

### 碰撞检测

基于 tile 索引的 4 级碰撞码：
- **Code 0** — 空（自由通过）
- **Code 1** — 地板（可站立、可穿墙跳下）
- **Code 2** — 水面（不可跳跃、下半身隐藏）
- **Code 3** — 固体（完全不可穿透）

---

## 快速开始

### 前置依赖

- **Windows 10/11** + **Visual Studio 2019**
- **NASM** — 汇编器
- **QEMU** — 用于运行测试
- **Python 3.x** — 用于数据提取和精灵转换工具

### 编译步骤

# 2. 转换精灵图片（PNG → .spr）
cd tools
python png_to_spr.py
cd ..

# 3. 编译 UEFI 应用程序
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

# 4. 部署并测试
cp Build/EmulatorX64/DEBUG_VS2019/X64/Contra.efi ../contra/qemu_test/
cd ../contra/qemu_test

qemu-system-x86_64 \
  -drive if=pflash,format=raw,file=OVMF_CODE.fd,readonly=on \
  -drive file=fat:rw:.,format=raw \
  -net none -m 256 -vga std
```

在 UEFI Shell 中输入 `Contra` 即可运行。

---

## 操作说明

| 按键 | 功能 |
|------|------|
| ← → | 水平移动 |
| ↑ ↓ | 瞄准方向 / 菜单选择 |
| Z | 射击 |
| X | 跳跃 |
| Enter | 开始 / 确认 |
| P | 暂停 |
| ESC | 退出游戏 |

---

## 设计约束与取舍

本项目力求在 UEFI 环境下忠实还原 NES 魂斗罗的核心体验，同时做出以下务实的取舍：

- **不包含音频** — UEFI 环境下音频驱动复杂度高，且 QEMU 模拟效果有限
- **静态内存分配** — 所有游戏数组编译期定长，避免运行时动态分配带来的碎片和失败
- **整数缩放** — 256×240 游戏画面通过 2×/3× 整数倍缩放至现代分辨率，避免非整数缩放导致的像素锯齿
- **精灵 PNG 预转换** — 精灵图片在编译前通过 Python 脚本转换为 `.spr` 二进制格式，运行时通过 UEFI 文件系统协议加载，避免将图片嵌入 .efi 导致体积膨胀
- **固定点物理** — 使用 INT32 8.8 定点数格式，避免浮点运算，确保跨平台精度一致

---

## 代码统计

| 类别 | 行数 |
|------|------|
| C 源代码 (.c) | ~3,200 |
| 头文件 (.h) | ~1,900 |
| 关卡数据 | ~650 |
| Python 工具 | ~1,800 |
| **总计** | **~7,500** |

---

## 参考资源

- [NES Contra US Disassembly](https://github.com/vermiceli/nes-contra-us) — 6502 汇编反编译项目
- [Retro Game Internals: Contra Levels](https://tomorrowcorporation.com/posts/retro-game-internals-contra-levels) — Allan Blomquist 的关卡结构深度解析
- [NESDev Wiki](https://www.nesdev.org/wiki/) — NES 图形（PPU）、调色板、Nametable 等技术规范

---

## 开源许可

本项目代码以 **MIT License** 授权发布。

NES 反汇编参考代码及精灵图片来源于 [nes-contra-us](https://github.com/vermiceli/nes-contra-us) 项目，版权归原始权利人所有。

---

## 鸣谢

- [vermiceli](https://github.com/vermiceli) 及贡献者 — 完整的 NES 魂斗罗反汇编工程，为移植提供了精确的游戏逻辑参考
- UEFI / TianoCore 社区 — EDK2 开发框架和 OVMF 固件
- QEMU 项目 — 强大的开源模拟器，使开发和测试成为可能

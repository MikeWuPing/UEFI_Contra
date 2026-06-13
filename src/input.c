#include "input.h"

CONTROLLER_STATE gController1;
CONTROLLER_STATE gController2;

VOID InputInit(VOID)
{
  ZeroMem(&gController1, sizeof(gController1));
  ZeroMem(&gController2, sizeof(gController2));
}

VOID PollInput(VOID)
{
  EFI_STATUS     Status;
  EFI_INPUT_KEY  Key;
  UINT8          newState1 = 0;
  UINT8          newState2 = 0;

  // Drain all pending key events
  while (TRUE) {
    Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
    if (EFI_ERROR(Status)) break;

    // Player 1 controls
    switch (Key.ScanCode) {
      case 0x0001: newState1 |= INPUT_UP;    break; // Arrow Up
      case 0x0002: newState1 |= INPUT_DOWN;  break; // Arrow Down
      case 0x0004: newState1 |= INPUT_LEFT;  break; // Arrow Left
      case 0x0003: newState1 |= INPUT_RIGHT; break; // Arrow Right
    }

    switch (Key.UnicodeChar) {
      case L'z': case L'Z': newState1 |= INPUT_B; break;    // Fire
      case L'x': case L'X': newState1 |= INPUT_A; break;    // Jump
      case L' ':            newState1 |= INPUT_A; break;    // Jump (alt)
      case 0x000D:          newState1 |= INPUT_START; break; // Enter = Start
      case L'a': case L'A': newState1 |= INPUT_B; break;    // Fire (alt)
      case L's': case L'S': newState1 |= INPUT_A; break;    // Jump (alt 2)

      // Player 2 controls (IJKL + U/O)
      case L'i': case L'I': newState2 |= INPUT_UP;    break;
      case L'k': case L'K': newState2 |= INPUT_DOWN;  break;
      case L'j': case L'J': newState2 |= INPUT_LEFT;  break;
      case L'l': case L'L': newState2 |= INPUT_RIGHT; break;
      case L'u': case L'U': newState2 |= INPUT_B;     break;
      case L'o': case L'O': newState2 |= INPUT_A;     break;

      case L'p': case L'P': break; // Pause (handled in game loop)
      case L'q': case L'Q': break; // Quit (handled in game loop)

      // ESC key
      case 0x001B: break;
    }

    // ESC scan code
    if (Key.ScanCode == 0x0017) {
      // Handled in game loop
    }
  }

  // Update controller 1 transitions
  gController1.Previous = gController1.Current;
  gController1.Current  = newState1;
  gController1.Pressed  = gController1.Current & ~gController1.Previous;
  gController1.Released = gController1.Previous & ~gController1.Current;

  // Update controller 2 transitions
  gController2.Previous = gController2.Current;
  gController2.Current  = newState2;
  gController2.Pressed  = gController2.Current & ~gController2.Previous;
  gController2.Released = gController2.Previous & ~gController2.Current;
}

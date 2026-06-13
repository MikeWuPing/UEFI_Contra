#ifndef INPUT_H
#define INPUT_H

#include "types.h"

// NES controller button bitmasks
#define INPUT_A        0x01
#define INPUT_B        0x02
#define INPUT_SELECT   0x04
#define INPUT_START    0x08
#define INPUT_UP       0x10
#define INPUT_DOWN     0x20
#define INPUT_LEFT     0x40
#define INPUT_RIGHT    0x80

// Controller state with edge detection
typedef struct {
  UINT8 Current;    // Buttons held this frame
  UINT8 Previous;   // Buttons held last frame
  UINT8 Pressed;    // Buttons just pressed (0->1 transition)
  UINT8 Released;   // Buttons just released (1->0 transition)
} CONTROLLER_STATE;

extern CONTROLLER_STATE gController1;
extern CONTROLLER_STATE gController2;

// Poll keyboard input and update controller states
VOID PollInput(VOID);

// Initialize input system
VOID InputInit(VOID);

#endif

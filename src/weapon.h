#ifndef WEAPON_H
#define WEAPON_H

#include "types.h"

// Weapon types (NES Contra)
#define WPN_DEFAULT  0
#define WPN_M        1  // Machine Gun
#define WPN_F        2  // Flame Thrower
#define WPN_S        3  // Spray/Spread
#define WPN_L        4  // Laser
#define WPN_B        5  // Barrier (invincibility)
#define WPN_R        6  // Rapid fire modifier

VOID UpdateWeaponItems(VOID);
VOID DrawWeaponItems(VOID);
VOID SpawnWeaponDrop(INT32 worldX, INT32 worldY, UINT8 weaponType);
VOID ClearWeaponItems(VOID);

#endif

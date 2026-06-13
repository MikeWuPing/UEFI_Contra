#include "types.h"

extern VOID ContraMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  Print(L"\n=== Contra UEFI Game ===\n");

  ContraMain(ImageHandle, SystemTable);

  Print(L"=== Contra UEFI Game Exit ===\n");
  return EFI_SUCCESS;
}

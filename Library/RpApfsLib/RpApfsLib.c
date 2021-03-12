/** @file
Copyright (C) 2020, vit9696. All rights reserved.

Modified 2021, Dayo Akanji. (sf.net/u/dakanji/profile)

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "RpApfsInternal.h"
#include "RpApfsLib.h"
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/PartitionInfo.h>

#include "../../include/refit_call_wrapper.h"

EFI_STATUS
RpApfsConnectParentDevice (
    VOID
) {
  EFI_STATUS       Status;
  EFI_STATUS       XStatus;
  UINTN            HandleCount;
  EFI_HANDLE       *HandleBuffer;
  EFI_DEVICE_PATH  *ParentDevicePath = NULL;
  EFI_DEVICE_PATH  *ChildDevicePath;
  UINTN            Index;
  UINTN            PrefixLength = 0;

  HandleCount = 0;
  Status = refit_call5_wrapper(
      gBS->LocateHandleBuffer,
      ByProtocol,
      &gEfiBlockIoProtocolGuid,
      NULL,
      &HandleCount,
      &HandleBuffer
  );

  if (!EFI_ERROR (Status)) {
      Status = EFI_NOT_FOUND;

      for (Index = 0; Index < HandleCount; ++Index) {
          XStatus = RpApfsConnectHandle (HandleBuffer[Index]);
          if (XStatus == EFI_SUCCESS || XStatus == EFI_ALREADY_STARTED) {
              if (EFI_ERROR (Status)) {
                  Status = XStatus;
              }
          }
      }

      FreePool (HandleBuffer);
  }

  return Status;
}

EFI_STATUS
RpApfsConnectDevices (
    VOID
) {
    EFI_STATUS  Status;
    VOID        *PartitionInfoInterface;


    Status = refit_call3_wrapper(
        gBS->LocateProtocol,
        &gEfiPartitionInfoProtocolGuid,
        NULL,
        &PartitionInfoInterface
    );
    Status = RpApfsConnectParentDevice();

    return Status;
}

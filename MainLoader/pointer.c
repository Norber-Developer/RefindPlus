/*
 * MainLoader/pointer.c
 * Pointer device functions
 *
 * Copyright (c) 2018 CJ Vaughter
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * Modified for RefindPlus
 * Copyright (c) 2020-2021 Dayo Akanji (sf.net/u/dakanji/profile)
 *
 * Modifications distributed under the preceding terms.
 */

#include "pointer.h"
#include "global.h"
#include "screenmgt.h"
#include "icns.h"
#include "../include/refit_call_wrapper.h"

EFI_HANDLE* APointerHandles = NULL;
EFI_ABSOLUTE_POINTER_PROTOCOL** APointerProtocol = NULL;
EFI_GUID APointerGuid = EFI_ABSOLUTE_POINTER_PROTOCOL_GUID;
UINTN NumAPointerDevices = 0;

EFI_HANDLE* SPointerHandles = NULL;
EFI_SIMPLE_POINTER_PROTOCOL** SPointerProtocol = NULL;
EFI_GUID SPointerGuid = EFI_SIMPLE_POINTER_PROTOCOL_GUID;
UINTN NumSPointerDevices = 0;

BOOLEAN PointerAvailable = FALSE;

UINTN LastXPos = 0, LastYPos = 0;
EG_IMAGE* MouseImage = NULL;
EG_IMAGE* Background = NULL;

POINTER_STATE State;

////////////////////////////////////////////////////////////////////////////////
// Initialize all pointer devices
////////////////////////////////////////////////////////////////////////////////
VOID pdInitialize() {
        #if REFIT_DEBUG > 0
        MsgLog ("Initialise Pointer Devices...\n");
        #endif

    pdCleanup(); // just in case

    if (! (GlobalConfig.EnableMouse || GlobalConfig.EnableTouch)) {
        #if REFIT_DEBUG > 0
        MsgLog ("  - Detected Touch Mode or 'No Mouse' Mode\n");
        #endif
    }
    else {
        // Get all handles that support absolute pointer protocol (usually touchscreens, but sometimes mice)
        UINTN NumPointerHandles = 0;
        EFI_STATUS handlestatus = refit_call5_wrapper(
            gBS->LocateHandleBuffer,
            ByProtocol,
            &APointerGuid,
            NULL,
            &NumPointerHandles,
            &APointerHandles
        );

        if (!EFI_ERROR (handlestatus)) {
            APointerProtocol = AllocatePool (sizeof (EFI_ABSOLUTE_POINTER_PROTOCOL*) * NumPointerHandles);
            UINTN Index;
            for (Index = 0; Index < NumPointerHandles; Index++) {
                // Open the protocol on the handle
                EFI_STATUS status = refit_call6_wrapper(
                    gBS->OpenProtocol,
                    APointerHandles[Index],
                    &APointerGuid,
                    (VOID **) &APointerProtocol[NumAPointerDevices],
                    SelfImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
                );
                if (status == EFI_SUCCESS) {
                    #if REFIT_DEBUG > 0
                    MsgLog ("  - Enable Touch\n");
                    #endif

                    NumAPointerDevices++;
                }
            }
        }
        else {
            #if REFIT_DEBUG > 0
            MsgLog ("  - Disable Touch\n");
            #endif

            GlobalConfig.EnableTouch = FALSE;
        }

        // Get all handles that support simple pointer protocol (mice)
        NumPointerHandles = 0;
        handlestatus = refit_call5_wrapper(
            gBS->LocateHandleBuffer,
            ByProtocol,
            &SPointerGuid,
            NULL,
            &NumPointerHandles,
            &SPointerHandles
        );

        if (!EFI_ERROR (handlestatus)) {
            SPointerProtocol = AllocatePool (sizeof (EFI_SIMPLE_POINTER_PROTOCOL*) * NumPointerHandles);
            UINTN Index;
            for (Index = 0; Index < NumPointerHandles; Index++) {
                // Open the protocol on the handle
                EFI_STATUS status = refit_call6_wrapper(
                    gBS->OpenProtocol,
                    SPointerHandles[Index],
                    &SPointerGuid,
                    (VOID **) &SPointerProtocol[NumSPointerDevices],
                    SelfImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
                );
                if (status == EFI_SUCCESS) {
                    #if REFIT_DEBUG > 0
                    MsgLog ("  - Enable Mouse\n");
                    #endif

                    NumSPointerDevices++;
                }
            }
        }
        else {

            #if REFIT_DEBUG > 0
            MsgLog ("  - Disable Mouse\n");
            #endif

            GlobalConfig.EnableMouse = FALSE;
        }

        PointerAvailable = (NumAPointerDevices + NumSPointerDevices > 0);

        // load mouse icon
        if (PointerAvailable && GlobalConfig.EnableMouse) {
            MouseImage = BuiltinIcon (BUILTIN_ICON_MOUSE);
        }
    }

    #if REFIT_DEBUG > 0
    MsgLog ("Pointer Devices Initialised\n\n");
    #endif
}

////////////////////////////////////////////////////////////////////////////////
// Frees allocated memory and closes pointer protocols
////////////////////////////////////////////////////////////////////////////////
VOID pdCleanup() {
        #if REFIT_DEBUG > 0
        MsgLog ("Close Existing Pointer Protocols:\n");
        #endif

    PointerAvailable = FALSE;
    pdClear();

    if (APointerHandles) {
        UINTN Index;
        for (Index = 0; Index < NumAPointerDevices; Index++) {
            refit_call4_wrapper(
                gBS->CloseProtocol,
                APointerHandles[Index],
                &APointerGuid,
                SelfImageHandle,
                NULL
            );
        }
        FreePool (APointerHandles);
        APointerHandles = NULL;
    }
    if (APointerProtocol) {
        FreePool (APointerProtocol);
        APointerProtocol = NULL;
    }
    if (SPointerHandles) {
        UINTN Index;
        for (Index = 0; Index < NumSPointerDevices; Index++) {
            refit_call4_wrapper(
                gBS->CloseProtocol,
                SPointerHandles[Index],
                &SPointerGuid,
                SelfImageHandle,
                NULL
            );
        }
        FreePool (SPointerHandles);
        SPointerHandles = NULL;
    }
    if (SPointerProtocol) {
        FreePool (SPointerProtocol);
        SPointerProtocol = NULL;
    }
    if (MouseImage) {
        egFreeImage (MouseImage);
        Background = NULL;
    }
    NumAPointerDevices = 0;
    NumSPointerDevices = 0;

    LastXPos = ScreenW / 2;
    LastYPos = ScreenH / 2;

    State.X = ScreenW / 2;
    State.Y = ScreenH / 2;
    State.Press = FALSE;
    State.Holding = FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Returns whether or not any pointer devices are available
////////////////////////////////////////////////////////////////////////////////
BOOLEAN pdAvailable() {
    return PointerAvailable;
}

////////////////////////////////////////////////////////////////////////////////
// Returns the number of pointer devices available
////////////////////////////////////////////////////////////////////////////////
UINTN pdCount() {
    return NumAPointerDevices + NumSPointerDevices;
}

////////////////////////////////////////////////////////////////////////////////
// Returns a pointer device's WaitForInput event
////////////////////////////////////////////////////////////////////////////////
EFI_EVENT pdWaitEvent (UINTN Index) {
    if (!PointerAvailable || Index >= NumAPointerDevices + NumSPointerDevices) {
        return NULL;
    }

    if (Index >= NumAPointerDevices) {
        return SPointerProtocol[Index - NumAPointerDevices]->WaitForInput;
    }
    return APointerProtocol[Index]->WaitForInput;
}

////////////////////////////////////////////////////////////////////////////////
// Gets the current state of all pointer devices and assigns State to
// the first available device's state
////////////////////////////////////////////////////////////////////////////////
EFI_STATUS pdUpdateState() {
#if defined (EFI32) && defined (__MAKEWITH_GNUEFI)
    return EFI_NOT_READY;
#else
    if (!PointerAvailable) {
        return EFI_NOT_READY;
    }

    EFI_STATUS Status = EFI_NOT_READY;
    EFI_ABSOLUTE_POINTER_STATE APointerState;
    EFI_SIMPLE_POINTER_STATE SPointerState;
    BOOLEAN LastHolding = State.Holding;

    UINTN Index;
    for (Index = 0; Index < NumAPointerDevices; Index++) {
        EFI_STATUS PointerStatus = refit_call2_wrapper(
            APointerProtocol[Index]->GetState,
            APointerProtocol[Index],
            &APointerState
        );
        // if new state found and we have not already found a new state
        if (!EFI_ERROR (PointerStatus) && EFI_ERROR (Status)) {
            Status = EFI_SUCCESS;

#ifdef EFI32
            State.X = (UINTN)DivU64x64Remainder (APointerState.CurrentX * ScreenW, APointerProtocol[Index]->Mode->AbsoluteMaxX, NULL);
            State.Y = (UINTN)DivU64x64Remainder (APointerState.CurrentY * ScreenH, APointerProtocol[Index]->Mode->AbsoluteMaxY, NULL);
#else
            State.X = (APointerState.CurrentX * ScreenW) / APointerProtocol[Index]->Mode->AbsoluteMaxX;
            State.Y = (APointerState.CurrentY * ScreenH) / APointerProtocol[Index]->Mode->AbsoluteMaxY;
#endif
            State.Holding = (APointerState.ActiveButtons & EFI_ABSP_TouchActive);
        }
    }
    for (Index = 0; Index < NumSPointerDevices; Index++) {
        EFI_STATUS PointerStatus = refit_call2_wrapper(
            SPointerProtocol[Index]->GetState,
            SPointerProtocol[Index],
            &SPointerState
        );
        // if new state found and we have not already found a new state
        if (!EFI_ERROR (PointerStatus) && EFI_ERROR (Status)) {
            Status = EFI_SUCCESS;

            INT32 TargetX = 0;
            INT32 TargetY = 0;

#ifdef EFI32
	    TargetX = State.X + (INTN)DivS64x64Remainder (
            SPointerState.RelativeMovementX * GlobalConfig.MouseSpeed,
            SPointerProtocol[Index]->Mode->ResolutionX,
            NULL
        );
            TargetY = State.Y + (INTN)DivS64x64Remainder (
                SPointerState.RelativeMovementY * GlobalConfig.MouseSpeed,
                SPointerProtocol[Index]->Mode->ResolutionY,
                NULL
            );
#else
            TargetX = State.X + SPointerState.RelativeMovementX *
                GlobalConfig.MouseSpeed / SPointerProtocol[Index]->Mode->ResolutionX;
            TargetY = State.Y + SPointerState.RelativeMovementY *
                GlobalConfig.MouseSpeed / SPointerProtocol[Index]->Mode->ResolutionY;
#endif

            if (TargetX < 0) {
                State.X = 0;
            }
            else if (TargetX >= ScreenW) {
                State.X = ScreenW - 1;
            }
            else {
                State.X = TargetX;
            }

            if (TargetY < 0) {
                State.Y = 0;
            }
            else if (TargetY >= ScreenH) {
                State.Y = ScreenH - 1;
            }
            else {
                State.Y = TargetY;
            }

            State.Holding = SPointerState.LeftButton;
        }
    }

    State.Press = (LastHolding && !State.Holding);

    return Status;
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Returns the current pointer state
////////////////////////////////////////////////////////////////////////////////
POINTER_STATE pdGetState() {
    return State;
}

////////////////////////////////////////////////////////////////////////////////
// Draw the mouse at the current coordinates
////////////////////////////////////////////////////////////////////////////////
VOID pdDraw() {
    if (Background) {
        egFreeImage (Background);
        Background = NULL;
    }
    if (MouseImage) {
        UINTN Width = MouseImage->Width;
        UINTN Height = MouseImage->Height;

        if (State.X + Width > ScreenW) {
            Width = ScreenW - State.X;
        }
        if (State.Y + Height > ScreenH) {
            Height = ScreenH - State.Y;
        }

        Background = egCopyScreenArea (State.X, State.Y, Width, Height);
        if (Background) {
            BltImageCompositeBadge (Background, MouseImage, NULL, State.X, State.Y);
        }
    }
    LastXPos = State.X;
    LastYPos = State.Y;
}

////////////////////////////////////////////////////////////////////////////////
// Restores the background at the position the mouse was last drawn
////////////////////////////////////////////////////////////////////////////////
VOID pdClear() {
    if (Background) {
        egDrawImage (Background, LastXPos, LastYPos);
        egFreeImage (Background);
        Background = NULL;
    }
}

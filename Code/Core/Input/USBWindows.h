// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
// 
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files(the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and / or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ********************************************************************
#pragma once
#include "Core/Common/Types.h"
#if defined(LF_OS_WINDOWS)
#include "Core/Common/API.h"
#include "Core/Utility/Array.h"

#include <Windows.h>
#include <hidsdi.h>
#include <setupapi.h>
namespace lf
{
namespace USB
{

// This is a fun experiment, but not necessary for now.
// If we do run into some cases where we can't get input for controllers
// we can come back to this code.
// 
// Essentially inputs are just streams of bytes. 
// We have to create an input device (handle to file) and then query 
// capabilities (input buffer length) and then read/parse input buffer.
// 
// It'll require us to study the input types of a specific device though

#if 0
    struct HID_DATA {
        bool        mIsButtonData;
        ByteT       mReserved;
        USAGE       mUsagePage;   // The usage page for which we are looking.
        ULONG       mStatus;      // The last status returned from the accessor function
                                    // when updating this field.
        ULONG       mReportID;    // ReportID for this given data structure
        bool        mIsDataSet;   // Variable to track whether a given data structure
                                    //  has already been added to a report structure
        TArray<USAGE> mButtonUsages;
        union {
            struct {
                ULONG       mUsageMin;       // Variables to track the usage minimum and max
                ULONG       mUsageMax;       // If equal, then only a single usage
                ULONG       mMaxUsageLength; // Usages buffer length.
                // PUSAGE      mUsages;         // list of usages (buttons ``down'' on the device.

            } mButtonData;
            struct {
                USAGE       mUsage; // The usage describing this value;
                USHORT      mReserved;

                ULONG       mValue;
                LONG        mScaledValue;
            } mValueData;
        };
    };

struct Win32Device
{
    HANDLE mHandle;
    PHIDP_PREPARSED_DATA mPPD;
    HIDD_ATTRIBUTES mAttributes;
    HIDP_CAPS mCaps;

    TArray<char> mInputReportBuffer;

    TArray<HIDP_BUTTON_CAPS> mInputButtonCaps;
    TArray<HIDP_VALUE_CAPS> mInputValueCaps;

    TArray<HID_DATA> mInputData;
};

struct Win32Handle
{
    HDEVINFO mDevInfo;
    GUID     mHidGUID;

    TArray<Win32Device> mDevices;
};

LF_CORE_API bool InitHandle(Win32Handle* handle);
LF_CORE_API bool ReleaseHandle(Win32Handle* handle);


LF_CORE_API TArray<SP_DEVICE_INTERFACE_DATA> GetInterfaceData(Win32Handle* handle);
LF_CORE_API TArray<SP_DEVINFO_DATA> GetDeviceInfoData(Win32Handle* handle);
LF_CORE_API TArray<SP_DRVINFO_DATA_A> GetDriverInfoData(Win32Handle* handle);

LF_CORE_API void Hook(Win32Handle* handle);
#endif
}
} // namespace lf
#endif // LF_OS_WINDOWS
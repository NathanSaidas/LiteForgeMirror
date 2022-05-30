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
#include "Core/PCH.h"
#include "USBWindows.h"
#if defined(LF_OS_WINDOWS)
#if 0

// References
// https://github.com/Jays2Kings/DS4Windows/blob/jay/DS4Windows/DS4Library/DS4Device.cs
// https://github.com/microsoft/Windows-driver-samples/tree/6c1981b8504329521343ad00f32daa847fa6083a/hid/hclient
// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf

#include "Core/Input/USB.h"
#include "Core/Utility/Log.h"

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

namespace lf
{

bool USB::InitHandle(Win32Handle* handle)
{
    if (!handle || handle->mDevInfo != INVALID_HANDLE_VALUE)
    {
        return handle;
    }

    HidD_GetHidGuid(&handle->mHidGUID);
    handle->mDevInfo = SetupDiGetClassDevs(&handle->mHidGUID, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    return handle->mDevInfo != INVALID_HANDLE_VALUE;
}

bool USB::ReleaseHandle(Win32Handle* handle)
{
    if (handle->mDevInfo == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    bool result = SetupDiDestroyDeviceInfoList(handle->mDevInfo) == TRUE;
    handle->mDevInfo = INVALID_HANDLE_VALUE;
    return result;
}

TArray<SP_DEVICE_INTERFACE_DATA> USB::GetInterfaceData(Win32Handle* handle)
{
    TArray<SP_DEVICE_INTERFACE_DATA> interfaceData;
    SizeT numInterfaces = 0;
    while (true)
    {
        SP_DEVICE_INTERFACE_DATA data;
        data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        if (FALSE == SetupDiEnumDeviceInterfaces(handle->mDevInfo, 0, &handle->mHidGUID, static_cast<DWORD>(numInterfaces), &data))
        {
            if (ERROR_NO_MORE_ITEMS != GetLastError())
            {
                interfaceData.Clear();
                numInterfaces = 0;
            }
            break;
        }
        interfaceData.Add(data);
        ++numInterfaces;
    }
    return interfaceData;
}
TArray<SP_DEVINFO_DATA> USB::GetDeviceInfoData(Win32Handle* handle)
{
    TArray<SP_DEVINFO_DATA> deviceData;
    SizeT numDevices = 0;
    while (true)
    {
        SP_DEVINFO_DATA data;
        data.cbSize = sizeof(SP_DEVINFO_DATA);
        if (FALSE == SetupDiEnumDeviceInfo(handle->mDevInfo, static_cast<DWORD>(numDevices), &data))
        {
            if (ERROR_NO_MORE_ITEMS != GetLastError())
            {
                deviceData.Clear();
                numDevices = 0;
            }
            break;
        }
        deviceData.Add(data);
        ++numDevices;
    }
    return deviceData;
}
TArray<SP_DRVINFO_DATA_A> USB::GetDriverInfoData(Win32Handle* handle)
{
    TArray<SP_DRVINFO_DATA_A> driverData;
    SizeT numInfo = 0;
    while (true)
    {
        SP_DRVINFO_DATA_A data;
        data.cbSize = sizeof(SP_DRVINFO_DATA_A);
        if(FALSE == SetupDiEnumDriverInfoA(handle->mDevInfo, nullptr, SPDIT_CLASSDRIVER, static_cast<DWORD>(numInfo), &data))
        {
            if (ERROR_NO_MORE_ITEMS != GetLastError())
            {
                driverData.Clear();
                numInfo = 0;
            }
            break;
        }
        driverData.Add(data);
        ++numInfo;
    }
    return driverData;
}

void CloseHidDevice(USB::Win32Device* device)
{
    if (device->mHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(device->mHandle);
        device->mHandle = INVALID_HANDLE_VALUE;
    }

    if (device->mPPD)
    {
        HidD_FreePreparsedData(device->mPPD);
        device->mPPD = nullptr;
    }
}

bool ReadDevice(USB::Win32Device* device, TArray<char>& data)
{
    if (device->mCaps.UsagePage != USB::UsagePage::USAGE_PAGE_GENERIC_DESKTOP_CONTROLS
        || device->mCaps.Usage != USB::UsageIdGenericDesktopControls::USAGE_ID_GAME_PAD)
    {
        return false;
    }

    DWORD bytesRead = 0;

    if (!ReadFile(
        device->mHandle,
        device->mInputReportBuffer.GetData(),
        static_cast<DWORD>(device->mInputReportBuffer.Size()),
        &bytesRead,
        NULL))
    {
        DWORD error = GetLastError(); (error);
        return false;
    }

    if (bytesRead != device->mInputReportBuffer.Size())
    {
        return false;
    }

    data = device->mInputReportBuffer;

    // SizeT reportIndex = 0;
    // for (SizeT i = 0; i < device->mInputData.Size(); ++i)
    // {
    //     ByteT reportID = reinterpret_cast<ByteT&>(device->mInputReportBuffer[reportIndex]);
    //     if (reportID == device->mInputData[i].mReportID)
    //     {
    // 
    //     }
    // }

    // USAGE usages[16];
    // ULONG numUsages = 16;
    // 
    // for (SizeT i = 0; i < device->mInputReportBuffer.Size(); ++i)
    // {
    // 
    // }
    // 
    // auto ReportType = HidP_Input;
    // auto status = HidP_GetUsages(
    //     HidP_Input,
    //     device->mCaps.UsagePage,
    //     0, // all collections
    //     usages,
    //     &numUsages,
    //     device->mPPD,
    //     device->mInputReportBuffer.GetData(),
    //     device->mInputReportBuffer.Size()
    // );

    return true;
}

void InitDevice(USB::Win32Device* device)
{
    device->mInputReportBuffer.Resize(device->mCaps.InputReportByteLength);

    device->mInputButtonCaps.Resize(device->mCaps.NumberInputButtonCaps);

    device->mInputValueCaps.Resize(device->mCaps.NumberInputValueCaps);

    NTSTATUS status = 0;

    USHORT numCaps = static_cast<USHORT>(device->mInputButtonCaps.Size());
    status = HidP_GetButtonCaps(HidP_Input, device->mInputButtonCaps.GetData(), &numCaps, device->mPPD);

    numCaps = static_cast<USHORT>(device->mInputValueCaps.Size());
    status = HidP_GetValueCaps(HidP_Input, device->mInputValueCaps.GetData(), &numCaps, device->mPPD);

    SizeT numValues = 0;
    for(HIDP_VALUE_CAPS& valueCaps : device->mInputValueCaps)
    {
        if (valueCaps.IsRange)
        {
            numValues += valueCaps.Range.UsageMax - valueCaps.Range.UsageMin + 1;
            if (valueCaps.Range.UsageMin > valueCaps.Range.UsageMax)
            {
                ReportBugMsg("Error!");
            }
        }
        else
        {
            ++numValues;
        }
    }

    device->mInputData.Resize(device->mCaps.NumberInputButtonCaps + numValues);
    auto data = device->mInputData.begin();
    for (SizeT i = 0; i < device->mCaps.NumberInputButtonCaps; ++i, ++data)
    {
        auto& buttonCaps = device->mInputButtonCaps[i];

        data->mIsButtonData = true;
        data->mStatus = HIDP_STATUS_SUCCESS;
        data->mUsagePage = buttonCaps.UsagePage;
        if (buttonCaps.IsRange)
        {
            data->mButtonData.mUsageMin = buttonCaps.Range.UsageMin;
            data->mButtonData.mUsageMax = buttonCaps.Range.UsageMax;
        }
        else
        {
            data->mButtonData.mUsageMin = data->mButtonData.mUsageMax = buttonCaps.NotRange.Usage;
        }

        data->mButtonData.mMaxUsageLength = HidP_MaxUsageListLength(HidP_Input, buttonCaps.UsagePage, device->mPPD);
        data->mButtonUsages.Resize(data->mButtonData.mMaxUsageLength);
        data->mReportID = buttonCaps.ReportID;
    }

    for (SizeT i = 0; i < device->mCaps.NumberInputValueCaps; ++i )
    {
        auto& valueCaps = device->mInputValueCaps[i];

        if (valueCaps.IsRange)
        {
            for (auto usage = valueCaps.Range.UsageMin; usage <= valueCaps.Range.UsageMax; ++usage)
            {
                if (data == device->mInputData.end())
                {
                    ReportBugMsg("Error");
                }
                data->mIsButtonData = false;
                data->mStatus = HIDP_STATUS_SUCCESS;
                data->mUsagePage = valueCaps.UsagePage;
                data->mValueData.mUsage = usage;
                data->mReportID = valueCaps.ReportID;
                ++data;
            }
        }
        else
        {
            if (data == device->mInputData.end())
            {
                ReportBugMsg("Error");
            }
            data->mIsButtonData = false;
            data->mStatus = HIDP_STATUS_SUCCESS;
            data->mUsagePage = valueCaps.UsagePage;
            data->mValueData.mUsage = valueCaps.NotRange.Usage;
            data->mReportID = valueCaps.ReportID;
            ++data;
        }
        
    }

}

bool OpenHidDevice(
    const char* devicePath,
    USB::Win32Device* device
)
{
    const bool hasReadAccess = true;
    const bool hasWriteAccess = false;
    const bool isOverlapped = false;
    const bool isExclusive = false;

    device->mHandle = INVALID_HANDLE_VALUE;
    device->mPPD = nullptr;

    DWORD accessFlags = 0;
    DWORD sharingFlags = 0;

    if (hasReadAccess) { accessFlags |= GENERIC_READ; }
    if (hasWriteAccess) { accessFlags |= GENERIC_WRITE; }
    if (!isExclusive) { sharingFlags = FILE_SHARE_READ | FILE_SHARE_WRITE; }

    device->mHandle = CreateFileA(devicePath,
        accessFlags,
        sharingFlags,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (device->mHandle == INVALID_HANDLE_VALUE)
    {
        gSysLog.Error(LogMessage("Failed to create file handle for input device...")
            << "\n  DeviceName: " << devicePath
        );
        return false;
    }

    if (!HidD_GetPreparsedData(device->mHandle, &device->mPPD))
    {
        CloseHidDevice(device);
        return false;
    }

    if (!HidD_GetAttributes(device->mHandle, &device->mAttributes))
    {
        CloseHidDevice(device);
        return false;
    }

    if(!HidP_GetCaps(device->mPPD, &device->mCaps))
    {
        CloseHidDevice(device);
        return false;
    }

    String usagePageStr = USB::UsagePage::GetEnum().ToString(device->mCaps.UsagePage);
    String usageStr = "";
    switch (device->mCaps.UsagePage)
    {
    case USB::UsagePage::USAGE_PAGE_GENERIC_DESKTOP_CONTROLS:
    {
        usageStr = USB::UsageIdGenericDesktopControls::GetEnum().ToString(device->mCaps.Usage);
    } break;
    case USB::UsagePage::USAGE_PAGE_CONSUMER:
    {
        usageStr = USB::UsageIdConsumer::GetEnum().ToString(device->mCaps.Usage);
    } break;
    case USB::UsagePage::USAGE_PAGE_TELEPHONY:
    {
        usageStr = USB::UsageIdTelephony::GetEnum().ToString(device->mCaps.Usage);
    } break;
    }

    if (usageStr.Empty())
    {
        SStream ss;
        usageStr = (ss << "Unknown(" << device->mCaps.Usage << ")").Str();
    }


    gSysLog.Info(LogMessage("Created input device...")
        << "\n  DeviceName: " << devicePath
        << "\n  UsagePage: " << usagePageStr
        << "\n  Usage: " << usageStr
        << "\n  InputReportLength: " << device->mCaps.InputReportByteLength
        << "\n  InputButtonCaps: " << device->mCaps.NumberInputButtonCaps
        << "\n  InputValueCaps: " << device->mCaps.NumberInputValueCaps
    );

    
    InitDevice(device);
    TArray<char> d0, d1;
    ReadDevice(device, d0);
    if (!d0.Empty())
    {
        for (SizeT i = 0; i < 2000; ++i)
        {
            Sleep(16);
            ReadDevice(device, d1);
            if (d1 != d0)
            {
                __debugbreak();
            }
        }
    }

    if (device->mCaps.UsagePage == USB::UsagePage::USAGE_PAGE_GENERIC_DESKTOP_CONTROLS
        && device->mCaps.Usage == USB::UsageIdGenericDesktopControls::USAGE_ID_MOUSE)
    {


        
    
        // ReadFile(device->mHandle, )
    }


    CloseHidDevice(device);

    return true;
}

void USB::Hook(Win32Handle* handle)
{
    if (handle || !handle)
    {
        return;
    }

    auto interfaceData = GetInterfaceData(handle);

    for (auto& data : interfaceData)
    {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(
            handle->mDevInfo,
            &data,
            nullptr,
            0,
            &requiredSize,
            nullptr);

        SP_DEVICE_INTERFACE_DETAIL_DATA* detail = (SP_DEVICE_INTERFACE_DETAIL_DATA*)LFAlloc(static_cast<SizeT>(requiredSize), 1);
        memset(detail, 0, static_cast<SizeT>(requiredSize));
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        DWORD predictedSize = requiredSize;

        if (SetupDiGetDeviceInterfaceDetailA(
            handle->mDevInfo,
            &data,
            detail,
            predictedSize,
            &requiredSize,
            NULL))
        {
            Win32Device device;
            if (!OpenHidDevice(detail->DevicePath, &device))
            {
                if (device.mHandle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(device.mHandle);
                    device.mHandle = INVALID_HANDLE_VALUE;
                }
            }
        }
        else
        {
            DWORD error = GetLastError();
            LF_DEBUG_BREAK;
            (error);
        }
        LFFree(detail);
    }

    
}

} // namespace lf
#endif
#endif // LF_OS_WINDOWS
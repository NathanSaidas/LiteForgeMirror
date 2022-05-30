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
#include "Core/Common/Enum.h"

#include <string.h>

namespace lf 
{

    

namespace USB
{

    

// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
// Page 14 (Universal Serial Bus HID Usage Tables)
namespace UsagePage
{
    using Value = UInt32;
    constexpr Value USAGE_PAGE_UNDEFINED = 0x00;
    constexpr Value USAGE_PAGE_GENERIC_DESKTOP_CONTROLS = 0x01;
    constexpr Value USAGE_PAGE_SIMULATION_CONTROLS = 0x02;
    constexpr Value USAGE_PAGE_VR_CONTROLS = 0x03;
    constexpr Value USAGE_PAGE_SPORT_COTNROLS = 0x04;
    constexpr Value USAGE_PAGE_GAME_CONTROLS = 0x05;
    constexpr Value USAGE_PAGE_GENERIC_DEVICE_CONTROLS = 0x06;
    constexpr Value USAGE_PAGE_KEYBOARD = 0x07;
    constexpr Value USAGE_PAGE_LED = 0x08;
    constexpr Value USAGE_PAGE_BUTTON = 0x09;
    constexpr Value USAGE_PAGE_ORDINAL = 0x0A;
    constexpr Value USAGE_PAGE_TELEPHONY = 0x0B;
    constexpr Value USAGE_PAGE_CONSUMER = 0x0C;
    constexpr Value USAGE_PAGE_DIGITIZER = 0x0D;
    constexpr Value USAGE_PAGE_RESERVED = 0x0E;
    constexpr Value USAGE_PAGE_PID_PAGE = 0x0F;

    auto& GetEnum()
    {
        static auto Enum = CreateEnumTable<Value, (Value)-1>(
            {
                { USAGE_PAGE_UNDEFINED, "USAGE_PAGE_UNDEIFNED" },
                { USAGE_PAGE_GENERIC_DESKTOP_CONTROLS, "USAGE_PAGE_GENERIC_DESKTOP_CONTROLS" },
                { USAGE_PAGE_SIMULATION_CONTROLS, "USAGE_PAGE_SIMULATION_CONTROLS" },
                { USAGE_PAGE_VR_CONTROLS, "USAGE_PAGE_VR_CONTROLS" },
                { USAGE_PAGE_SPORT_COTNROLS, "USAGE_PAGE_SPORT_COTNROLS" },
                { USAGE_PAGE_GAME_CONTROLS, "USAGE_PAGE_GAME_CONTROLS" },
                { USAGE_PAGE_GENERIC_DEVICE_CONTROLS, "USAGE_PAGE_GENERIC_DEVICE_CONTROLS" },
                { USAGE_PAGE_KEYBOARD, "USAGE_PAGE_KEYBOARD" },
                { USAGE_PAGE_LED, "USAGE_PAGE_LED" },
                { USAGE_PAGE_BUTTON, "USAGE_PAGE_BUTTON" },
                { USAGE_PAGE_ORDINAL, "USAGE_PAGE_ORDINAL" },
                { USAGE_PAGE_TELEPHONY, "USAGE_PAGE_TELEPHONY" },
                { USAGE_PAGE_CONSUMER, "USAGE_PAGE_CONSUMER" },
                { USAGE_PAGE_DIGITIZER, "USAGE_PAGE_DIGITIZER" },
                { USAGE_PAGE_RESERVED, "USAGE_PAGE_RESERVED" },
                { USAGE_PAGE_PID_PAGE, "USAGE_PAGE_PID_PAGE" }
            });
        return Enum;
    }
} // namespace UsagePage

// Page 26 (Universal Serial Bus HID Usage Tables)
namespace UsageIdGenericDesktopControls
{
    using Value = UInt32;
    constexpr Value USAGE_ID_UNDEFINED  = 0x00;
    // CP - A collection of axes that generates a value to direct, indicate, or point user intentions to an application..
    constexpr Value USAGE_ID_POINTER    = 0x01;
    // CA - 
    constexpr Value USAGE_ID_MOUSE = 0x02;
    // CA - Manual control or cursor device. Minimally consists of 2 axes (X and Y) and 2 buttons.
    // Normally rangeds from 0 to 64000 where 32000 is centered
    // Drivers typically normalize this from 0...1
    constexpr Value USAGE_ID_JOYSTICK = 0x04;
    // CA - Manual control or cursor device. Minimally consists of thumb-activated rocker switch that controls two axes 
    // (X and Y) and has four buttons.
    constexpr Value USAGE_ID_GAME_PAD = 0x05;
    // CA - 
    constexpr Value USAGE_ID_KEYBOARD = 0x06;
    constexpr Value USAGE_ID_KEYPAD = 0x07;
    // CA - Orient eyepoints and or objects in 3 dimensional space. Typically consists of 3 axes (6 variables)
    // (X, Y, Z, Rx, Ry, Rz) 
    constexpr Value USAGE_ID_MULTI_AXIS_CONTROLLER = 0x08;

    auto& GetEnum()
    {
        static auto Enum = CreateEnumTable<Value, (Value)-1>(
        {
            { USAGE_ID_UNDEFINED, "USAGE_ID_UNDEFINED" },
            { USAGE_ID_POINTER, "USAGE_ID_POINTER" },
            { USAGE_ID_MOUSE, "USAGE_ID_MOUSE" },
            { USAGE_ID_JOYSTICK, "USAGE_ID_JOYSTICK" },
            { USAGE_ID_GAME_PAD, "USAGE_ID_GAME_PAD" },
            { USAGE_ID_KEYBOARD, "USAGE_ID_KEYBOARD" },
            { USAGE_ID_KEYPAD, "USAGE_ID_KEYPAD" },
            { USAGE_ID_MULTI_AXIS_CONTROLLER, "USAGE_ID_MULTI_AXIS_CONTROLLER" },
        });
        return Enum;
    }
} // namespace UsageIdGenericDesktopControls

namespace UsageIdTelephony
{
    using Value = UInt32;
    constexpr Value USAGE_ID_UNASSIGNED = 0x00;
    constexpr Value USAGE_ID_PHONE = 0x01;
    constexpr Value USAGE_ID_ANSWERING_MACHINE = 0x02;
    constexpr Value USAGE_ID_MESSAGE_CONTROLS = 0x03;
    constexpr Value USAGE_ID_HANDSET = 0x04;
    constexpr Value USAGE_ID_HEADSET = 0x05;

    auto& GetEnum()
    {
        static auto Enum = CreateEnumTable<Value, (Value)-1>(
            {
                { USAGE_ID_UNASSIGNED, "USAGE_ID_UNASSIGNED" },
                { USAGE_ID_PHONE, "USAGE_ID_PHONE" },
                { USAGE_ID_ANSWERING_MACHINE, "USAGE_ID_ANSWERING_MACHINE" },
                { USAGE_ID_MESSAGE_CONTROLS, "USAGE_ID_MESSAGE_CONTROLS" },
                { USAGE_ID_HANDSET, "USAGE_ID_HANDSET" },
                { USAGE_ID_HEADSET, "USAGE_ID_HEADSET" },
            });
        return Enum;
    }
}

namespace UsageIdConsumer
{
    using Value = UInt32;
    constexpr Value USAGE_ID_UNASSIGNED = 0x00;
    constexpr Value USAGE_ID_CONSUMER_CONTROL = 0x01;
    constexpr Value USAGE_ID_MICROPHONE = 0x04;
    constexpr Value USAGE_ID_HEADPHONE = 0x05;

    auto& GetEnum()
    {
        static auto Enum = CreateEnumTable<Value, (Value)-1>(
            {
                { USAGE_ID_UNASSIGNED, "USAGE_ID_UNASSIGNED" },
                { USAGE_ID_CONSUMER_CONTROL, "USAGE_ID_CONSUMER_CONTROL" },
                { USAGE_ID_MICROPHONE, "USAGE_ID_MICROPHONE" },
                { USAGE_ID_HEADPHONE, "USAGE_ID_HEADPHONE" },
            });
        return Enum;
    }
}



} // namespace USB


} // namespace lf
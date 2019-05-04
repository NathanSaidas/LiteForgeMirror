// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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
#include "SStreamTest.h"
#include "Core/String/SStream.h"
#include "Core/String/StringCommon.h"
#include <Windows.h>

namespace lf {
REGISTER_TEST(SStreamTest)
{
    SStream ss;
    ss << "Hello Kris\n";
    ss << StreamFillLeft(16);
    ss << "Hello Kris\n";
    ss << StreamFillRight(16);
    ss << "Hello Kris\n";
    ss << StreamBoolAlpha(false);
    auto state = ss.Push();

    ss << "Lets build a table\n";
    ss << "+" << StreamFillRight(8) << StreamFillChar('-') << "+" << "+" << StreamFillLeft() << StreamFillChar(' ') << "\n";
    ss << "|" << StreamFillRight(8) << "|" << "|" << StreamFillLeft() << "\n";
    ss << "|" << StreamFillRight(8) << "|" << "|" << StreamFillLeft() << "\n";
    ss << "|" << StreamFillRight(8) << "|" << "|" << StreamFillLeft() << "\n";
    ss << "|" << StreamFillRight(8) << "|" << "|" << StreamFillLeft() << "\n";
    ss << "|" << StreamFillRight(8) << "|" << "|" << StreamFillLeft() << "\n";
    ss << "|" << StreamFillRight(8) << "|" << "|" << StreamFillLeft() << "\n";

    ss.Pop(state);
    ss << "Using previous table state=" << true << "\n";

    ss.Push();
    ss << "Alright let's write default again.. But encode a hex character!" << ToHexString(0xBADF00D) << "\n";




    
    OutputDebugString(ss.CStr());
    LF_DEBUG_BREAK;
}

} // namespace lf
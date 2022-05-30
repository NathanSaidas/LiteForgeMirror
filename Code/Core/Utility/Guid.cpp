// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
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
#include "Guid.h"
#include "Core/String/String.h"
#include "Core/String/StringUtil.h"
#include "Core/String/StringCommon.h"

namespace lf {

    bool ToGuid(const String& id, ByteT* data, const SizeT dataSize)
    {
        if (id.Size() != dataSize * 2)
        {
            return false; // bad guid:
        }

        char buffer[2];
        SizeT bufferLength = 0;
        SizeT dataIndex = 0;
        for (SizeT i = 0; i < id.Size(); ++i)
        {
            buffer[bufferLength++] = id[i];
            if (bufferLength >= 2)
            {
                if (!CharIsHex(buffer[0]) || !CharIsHex(buffer[1]))
                {
                    return false;
                }
                ByteT highByte = HexToByte(buffer[0]);
                ByteT lowByte = HexToByte(buffer[1]);
                highByte = highByte << 4;
                data[dataIndex] = lowByte | highByte;

                bufferLength = 0;
                ++dataIndex;
            }
        }
        return true;
    }

    String ToString(const ByteT* data, const SizeT dataSize)
    {
        const char* HEX_TABLE = "0123456789ABCDEF";
        String str;
        for (SizeT i = 0; i < dataSize; ++i)
        {
            ByteT lowByte = data[i] & 0xF;
            ByteT highByte = (data[i] & 0xF0) >> 4;

            str.Append(HEX_TABLE[highByte]);
            str.Append(HEX_TABLE[lowByte]);
        }
        return str;
    }

} // namespace lf
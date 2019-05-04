// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
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
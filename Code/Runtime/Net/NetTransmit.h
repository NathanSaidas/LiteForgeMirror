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
#pragma once

#include "Core/Net/NetTypes.h"
#include "Core/Utility/Array.h"

namespace lf {
    
class LF_RUNTIME_API NetTransmitInfo
{
public:
    NetTransmitInfo() : mData(0) {}
    NetTransmitInfo(UInt32 id, UInt32 crc32) : mData(0) { SetID(id), SetCrc32(crc32); }

    LF_INLINE UInt32 GetID() const { return reinterpret_cast<const UInt32*>(&mData)[0]; }
    LF_INLINE UInt32 GetCrc32() const { return reinterpret_cast<const UInt32*>(&mData)[1]; }
    LF_INLINE void SetID(UInt32 value) { reinterpret_cast<UInt32*>(&mData)[0] = value; }
    LF_INLINE void SetCrc32(UInt32 value) { reinterpret_cast<UInt32*>(&mData)[1] = value; }

    LF_INLINE UInt64 Value() const { return mData; }
    LF_INLINE bool Empty() const { return mData == 0; }
private:
    UInt64 mData;
};

using NetTransmitInfoArray = TVector<NetTransmitInfo>;

class LF_RUNTIME_API NetTransmitBuffer
{
public:

    void Resize(SizeT size) { mBuffer.resize(size); /*mBuffer.Collapse(); */ } // todo: Shrink to fit
    void Clear() { mBuffer.clear(); }
    SizeT Size() const { return mBuffer.size(); }
    bool Empty() const { return mBuffer.empty(); }

    bool Update(NetTransmitInfo info)
    {
        if (info.Empty() || Empty())
        {
            return false;
        }

        // Calc Index
        UInt32 index = info.GetID() % Size();

        // Fail to update if we're retransmitting the same info
        NetTransmitInfo other = mBuffer[index];
        if (other.Value() == info.Value())
        {
            return false;
        }
        mBuffer[index] = info;
        return true;
    }
private:
    NetTransmitInfoArray mBuffer;
};

} // namespace lf
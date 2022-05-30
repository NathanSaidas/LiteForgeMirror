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

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/Utility/StdVector.h"

namespace lf {

// This is a data structure that provides a guarantee on unique number generation
// 
// Numbers are generated starting from 0 and incrementing upwards.
// Can only generate maximum numbers based on the data type of T
// 
// Size=Stack Size of free list
template<typename T, SizeT SIZE>
class UniqueNumber
{
public:
    UniqueNumber()
        : mFreeList()
        , mTop(0)
    {}
    UniqueNumber(UniqueNumber&& other)
        : mFreeList(std::move(other.mFreeList))
        , mTop(other.mTop)
    {
        other.mTop = 0;
    }
    UniqueNumber(const UniqueNumber& other)
        : mFreeList(other.mFreeList)
        , mTop(other.mTop)
    {
    }
    ~UniqueNumber()
    {
    }

    UniqueNumber& operator=(UniqueNumber&& other)
    {
        if (this == &other)
        {
            return *this;
        }
        mFreeList = std::move(other.mFreeList);
        mTop = other.mTop;
        other.mTop = 0;
        return *this;
    }
    UniqueNumber& operator=(const UniqueNumber& other)
    {
        if (this == &other)
        {
            return *this;
        }
        mFreeList = other.mFreeList;
        mTop = other.mTop;
        return *this;
    }

    T Allocate()
    {
        T result;
        if (!mFreeList.empty())
        {
            result = mFreeList.back();
            mFreeList.pop_back();
            return result;
        }
        return mTop++;
    }

    void Free(T number)
    {
        if (number == (mTop - 1))
        {
            --mTop;
        }
        else
        {
#if defined(LF_DEBUG) || defined(LF_TEST)
            Assert(mFreeList.end() == std::find(mFreeList.begin(), mFreeList.end(), number));
#endif
            mFreeList.push_back(number);
        }
    }
private:
    TStackVector<T, SIZE> mFreeList;
    T mTop;
};

} // namespace lf
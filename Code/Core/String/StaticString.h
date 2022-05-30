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

namespace lf {

// Static String storage type. 
template<typename T, const SizeT TSize>
struct StaticStringType
{
    StaticStringType(const T(&errorString)[TSize])
        : mString(errorString)
    {
    }

    const T* CStr() const { return mString; }
    constexpr static SizeT Size() { return TSize; }


private:
    const T(&mString)[TSize];
};

// Utility function to make writing static strings a bit easier.
// 
// Usage: StaticString("Hello string that has compile time size!");
template<typename T, const SizeT TSize>
StaticStringType<T, TSize> StaticString(const T(&errorString)[TSize])
{
    return StaticStringType<T, TSize>(errorString);
}
} // namespace lf
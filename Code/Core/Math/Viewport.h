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

namespace lf {

template<typename ScalarT>
struct TViewport
{
    using scalar_type = ScalarT;

    TViewport()
        : left(scalar_type(0))
        , right(scalar_type(0))
        , top(scalar_type(0))
        , bottom(scalar_type(0))
        , near(scalar_type(0))
        , far(scalar_type(0))
    {}

    TViewport(LazyTag) {}

    TViewport(scalar_type width, scalar_type height)
        : left(scalar_type(0))
        , right(width)
        , top(height)
        , bottom(scalar_type(0))
        , near(scalar_type(0))
        , far(scalar_type(1000))
    {}

    TViewport(scalar_type width, scalar_type height, scalar_type depth)
        : left(scalar_type(0))
        , right(width)
        , top(height)
        , bottom(scalar_type(0))
        , near(scalar_type(0))
        , far(depth)
    {}

    TViewport(scalar_type left_, scalar_type right_, scalar_type top_, scalar_type bottom_, scalar_type near_, scalar_type far_)
        : left(left_)
        , right(right_)
        , top(top_)
        , bottom(bottom_)
        , near(near_)
        , far(far_)
    {}

    bool IsValid() const
    {
        return right >= left && top >= bottom && far = > near;
    }

    scalar_type left;
    scalar_type right;
    scalar_type top;
    scalar_type bottom;
    scalar_type near;
    scalar_type far;
};

using ViewportF = TViewport<Float32>;
using ViewportI = TViewport<Int32>;


} // namespace lf
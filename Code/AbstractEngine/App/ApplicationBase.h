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
#ifndef LF_ABSTRACT_ENGINE_APPLICATION_BASE_H
#define LF_ABSTRACT_ENGINE_APPLICATION_BASE_H

#include "Core/Common/API.h"
#include "Core/Reflection/Object.h"
#include "Runtime/Reflection/ReflectionTypes.h"

namespace lf {

class LF_ABSTRACT_ENGINE_API ApplicationBase : public Object
{
    DECLARE_CLASS(ApplicationBase, Object);
public:
    // **********************************
    // Called when the application is first started up. (After the bootstrap layer is initialized)
    // 
    // **********************************
    virtual void OnStart() = 0;
    // **********************************
    // Called just as the application is shutting down. (Before the bootstrap layer is terminated)
    // **********************************
    virtual void OnExit() = 0;
};

} // namespace lf

#endif // LF_ABSTRACT_ENGINE_APPLICATION_BASE_H
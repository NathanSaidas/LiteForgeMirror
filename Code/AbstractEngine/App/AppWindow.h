// ********************************************************************
// Copyright (c) 2021 Nathan Hanlan
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

#include "Core/Reflection/Object.h"
#include "Runtime/Reflection/ReflectionTypes.h"

namespace lf {

struct AppWindowDesc
{
    String mID;
    String mTitle;
    SizeT mWidth = 640;
    SizeT mHeight = 640;
    bool mDefaultHidden = false;
};
class ServiceContainer;

class LF_ABSTRACT_ENGINE_API AppWindow : public Object, public TAtomicWeakPointerConvertible<AppWindow>
{
    DECLARE_CLASS(AppWindow, Object);
public:
    using PointerConvertible = PointerConvertibleType;

    virtual ~AppWindow();
    virtual void InitDependencies(const ServiceContainer& services) = 0;
    virtual bool Create(const AppWindowDesc& desc) = 0;
    virtual void Destroy() = 0;
    virtual bool Show() = 0;
    virtual bool Hide() = 0;
    virtual void SetTitle(const String& title) = 0;
    virtual void SetSize(SizeT width, SizeT height) = 0;
    virtual bool IsOpen() const = 0;

    const String& GetID() const { return mID; }
    const String& GetTitle() const { return mTitle; }
    SizeT GetWidth() const { return mWidth; }
    SizeT GetHeight() const { return mHeight; }
protected:

    void UpdateID(const String& value) { mID = value; }
    void UpdateTitle(const String& value) { mTitle = value; }
    void UpdateWidth(SizeT value) { mWidth = value; }
    void UpdateHeight(SizeT value) { mHeight = value; }

    const Token& GetKeyboardDeviceName();
    const Token& GetMouseDeviceName();
private:
    String mID;
    String mTitle;
    SizeT mWidth;
    SizeT mHeight;
};

} // namespace lf
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
#include "Core/Reflection/Type.h"
#include "Core/Utility/StdVector.h"
#include "Runtime/Reflection/ReflectionTypes.h"

namespace lf
{

class LF_ABSTRACT_ENGINE_API AppConfigObject : public Object
{
    DECLARE_CLASS(AppConfigObject, Object);
public:
    virtual ~AppConfigObject() = default;
};
DECLARE_PTR(AppConfigObject);

class LF_ABSTRACT_ENGINE_API AppConfig
{
public:
    bool Read(const String& filepath);
    bool Write(const String& filepath);

    void Serialize(Stream& s);

    AppConfigObject* GetConfig(const Type* type) const;

private:
    void AddDefaultTypes();
    void SerializeCommon(Stream& s);

    struct DynamicConfigObject
    {
        DynamicConfigObject()
        : mType(nullptr)
        , mObject()
        {}
        void Serialize(Stream& s);

        const Type*         mType;
        AppConfigObjectPtr  mObject;

        friend Stream& operator<<(Stream& s, DynamicConfigObject& o)
        {
            o.Serialize(s);
            return s;
        }
    };

    TVector<DynamicConfigObject> mObjects;
};


} // namespace lf
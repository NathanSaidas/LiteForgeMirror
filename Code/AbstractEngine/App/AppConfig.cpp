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
#include "AbstractEngine/PCH.h"
#include "AppConfig.h"
#include "Runtime/Reflection/ReflectionMgr.h"

#include "Core/Platform/File.h"
#include "Core/IO/JsonStream.h"
#include "Core/Utility/Log.h"

namespace lf
{
DEFINE_ABSTRACT_CLASS(lf::AppConfigObject) { NO_REFLECTION; }

bool AppConfig::Read(const String& filepath)
{
    File file;
    if (!file.Open(filepath, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING))
    {
        gSysLog.Error(LogMessage("Failed to open AppConfig for reading. ") << filepath);
        return false;
    }

    String text;
    text.Resize(static_cast<SizeT>(file.GetSize()));
    if (!file.Read(const_cast<char*>(text.CStr()), text.Size()) == text.Size())
    {
        gSysLog.Error(LogMessage("Failed to read AppConfig. ") << filepath);
        return false;
    }

    JsonStream js(Stream::TEXT, &text, Stream::SM_READ);
    SerializeCommon(js);
    js.Close();

    AddDefaultTypes();
    return true;
}

bool AppConfig::Write(const String& filepath)
{
    AddDefaultTypes();

    String text;
    JsonStream js(Stream::TEXT, &text, Stream::SM_PRETTY_WRITE);
    SerializeCommon(js);
    js.Close();

    if (text.Empty())
    {
        text = "{}";
    }

    File file;
    if (!file.Open(filepath, FF_WRITE | FF_SHARE_READ, FILE_OPEN_CREATE_NEW))
    {
        gSysLog.Error(LogMessage("Failed to open AppConfig for writing. ") << filepath);
        return false;
    }

    if (file.Write(text.CStr(), text.Size()) != text.Size())
    {
        gSysLog.Error(LogMessage("Failed to write AppConfig. ") << filepath);
        return false;
    }
    return true;
}

void AppConfig::Serialize(Stream& s)
{
    SERIALIZE_STRUCT_ARRAY(s, mObjects, "");
}
AppConfigObject* AppConfig::GetConfig(const Type* type) const
{
    for (const auto& config : mObjects)
    {
        if (config.mType == type)
        {
            return config.mObject;
        }
    }
    return nullptr;
}

void AppConfig::AddDefaultTypes()
{
    TVector<const Type*> types = GetReflectionMgr().FindAll(typeof(AppConfigObject));
    for (const Type* type : types)
    {
        if (type->IsAbstract())
        {
            continue;
        }

        auto exist = std::find_if(mObjects.begin(), mObjects.end(), [type](const DynamicConfigObject& o)
            {
                return o.mType == type;
            });
        if (exist == mObjects.end())
        {
            DynamicConfigObject o;
            o.mType = type;
            o.mObject = GetReflectionMgr().Create<AppConfigObject>(type);
            mObjects.push_back(o);
        }
    }
}

void AppConfig::SerializeCommon(Stream& s)
{
    if (s.BeginObject("AppConfig", "BaseConfig"))
    {
        Serialize(s);
        s.EndObject();
    }
}

void AppConfig::DynamicConfigObject::Serialize(Stream& s)
{
    SERIALIZE(s, mType, "");
    if (s.IsReading())
    {
        mObject = mType ? GetReflectionMgr().Create<AppConfigObject>(mType) : NULL_PTR;
        if (mObject && (s << StreamPropertyInfo("Data")).BeginStruct())
        {
            mObject->Serialize(s);
            s.EndStruct();
        }
    }
    else
    {
        if (mObject) 
        {
            (s << StreamPropertyInfo("Data")).BeginStruct();
            mObject->Serialize(s);
            s.EndStruct();
        }
        else
        {
            (s << StreamPropertyInfo("Data")).BeginStruct();
            s.EndStruct();
        }
    }

}

} // namespace lf
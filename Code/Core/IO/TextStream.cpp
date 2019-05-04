#include "TextStream.h"
#include "Core/String/Token.h"
#include "Core/String/StringCommon.h"
#include "Core/String/StringUtil.h"
#include "Core/Platform/File.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Guid.h"
#include "Core/Reflection/Type.h"
#include "Core/Runtime/ReflectionHooks.h"



// #include "core/system/Log.h"
// #include "core/object/Object.h"
// #include "core/object/ReflectionMgr.h"
// #include "core/object/Type.h"
// #include "core/util/Guid.h"
#include <algorithm>


namespace lf
{
    static const String::value_type TOK_BEGIN_OBJECT = '$';
    static const String::value_type TOK_STREAM_VAR = '@';
    static const String::value_type TOK_BEGIN_STRUCT = '{';
    static const String::value_type TOK_END_STRUCT = '}';
    static const String::value_type TOK_BEGIN_ARRAY = '[';
    static const String::value_type TOK_END_ARRAY = ']';
    static const String::value_type TOK_PROPERTY_SEPARATOR = '=';
    static const size_t FLOAT_PRECISION = 8;


    StreamObject::StreamObject() :
        mType(),
        mSuper(),
        mProperties(),
        mBoundProperty(),
        mSelf()
    {

    }
    StreamObject::~StreamObject()
    {

    }

    void StreamObject::AddProperty(const StreamPropertyPtr& property)
    {
        property->context = mSelf;
        if (mBoundProperty)
        {
            property->parent = mBoundProperty;
            mBoundProperty->children.Add(property);
        }
        else
        {
            mProperties.Add(property);
        }

    }
    void StreamObject::RemoveProperty(const StreamPropertyPtr& property)
    {
        if (!property)
        {
            return;
        }
        if (property->parent)
        {
            TArray<StreamPropertyPtr>::iterator it = std::find(property->parent->children.begin(), property->parent->children.end(), property);
            if (it != property->parent->children.end())
            {
                property->parent->children.SwapRemove(it);
            }
        }
        else
        {
            TArray<StreamPropertyPtr>::iterator it = std::find(mProperties.begin(), mProperties.end(), property);
            if (it != mProperties.end())
            {
                mProperties.SwapRemove(it);
            }
        }
        if (property.AsPtr() == mBoundProperty)
        {
            mBoundProperty = NULL_PTR;
        }
    }
    void StreamObject::RemoveProperty(const String& name)
    {
        RemoveProperty(FindProperty(name));
    }
    void StreamObject::Clear()
    {
        mProperties.Clear();
    }

    struct FindPropertyByName
    {
        FindPropertyByName(const String& inName) : name(inName) {}

        bool operator()(const StreamPropertyWPtr& prop) const
        {
            return prop->name == name;
        }

        String name;
    };

    StreamPropertyWPtr StreamObject::FindBoundProperty(const String& name)
    {
        if (mBoundProperty)
        {
            auto it = std::find_if(mBoundProperty->children.begin(), mBoundProperty->children.end(), FindPropertyByName(name));
            if (it != mBoundProperty->children.end())
            {
                return *it;
            }
        }
        return NULL_PTR;
    }

    StreamPropertyWPtr StreamObject::FindProperty(const String& name)
    {
        TArray<String> tokens;
        StrSplit(name, '.', tokens);
        if (tokens.Empty())
        {
            return NULL_PTR;
        }
        if (tokens.Size() == 1)
        {
            auto it = std::find_if(mProperties.begin(), mProperties.end(), FindPropertyByName(name));
            if (it != mProperties.end())
            {
                return *it;
            }
            return NULL_PTR;
        }
        else
        {
            FindPropertyByName pred(tokens[0]);
            StreamPropertyWPtr prop = NULL_PTR;
            for (size_t cursor = 0; cursor < tokens.Size(); ++cursor)
            {
                if (cursor == 0)
                {
                    auto it = std::find_if(mProperties.begin(), mProperties.end(), pred);
                    if (it == mProperties.end())
                    {
                        break;
                    }
                    prop = *it;
                }
                else
                {
                    pred.name = tokens[cursor];
                    auto it = std::find_if(prop->children.begin(), prop->children.end(), pred);
                    if (it == prop->children.end())
                    {
                        prop = NULL_PTR;
                        break;
                    }
                    prop = *it;
                }
            }

            return prop;
        }
    }

    bool StreamObject::BindProperty(size_t index)
    {
        if (index < mProperties.Size())
        {
            mBoundProperty = mProperties[index];
            return true;
        }
        return false;
    }
    void StreamObject::BindProperty(const StreamPropertyWPtr& property)
    {
        Assert(!property || property->context == mSelf);
        mBoundProperty = property;
    }


    void StreamObject::Unbind()
    {
        mBoundProperty = NULL_PTR;
    }

    TextStream::TextStream() : Stream(), mContext(nullptr)
    {

    }
    TextStream::TextStream(Stream::StreamText, String* text, StreamMode mode) : Stream(), mContext(nullptr)
    {
        Open(Stream::TEXT, text, mode);
    }
    TextStream::TextStream(Stream::StreamFile, const String& filename, StreamMode mode) : Stream(), mContext(nullptr)
    {
        Open(Stream::FILE, filename, mode);
    }
    TextStream::TextStream(StreamContext*& context) : Stream(), mContext(nullptr)
    {
        SetContext(context);
        context = nullptr;
    }
    TextStream::~TextStream()
    {
        Close();
    }

    void TextStream::Open(Stream::StreamText, String* text, StreamMode mode)
    {
        if (!mContext)
        {
            void* memory = AllocContext(sizeof(TextStreamContext), alignof(TextStreamContext));
            mContext = new(memory)TextStreamContext();
            mContext->mType = StreamContext::TEXT;
            mContext->mMode = mode;
            mContext->mOutputText = nullptr;
            Assert(memory == mContext);
        }
        else
        {
            Clear();
        }

        mContext->mMode = mode;
        if (mode == SM_READ && text && !text->Empty())
        {
            ReadAllText(*text);
        }
        else if (mode == SM_WRITE)
        {
            mContext->mOutputText= text;
        }
        mContext->mModeStack.push(PM_NONE);
        // if (mContext->mode == SM_READ_APPEND)
        // {
        //     mContext->mode = SM_WRITE;
        // }
    }
    void TextStream::Open(Stream::StreamFile, const String& filename, StreamMode mode)
    {
        if (!mContext)
        {
            void* memory = AllocContext(sizeof(TextStreamContext), alignof(TextStreamContext));
            mContext = new(memory)TextStreamContext();
            mContext->mType = StreamContext::TEXT;
            mContext->mMode = mode;
            mContext->mOutputText = nullptr;
            Assert(memory == mContext);
        }
        else
        {
            Clear();
        }
        mContext->mMode = mode;
        mContext->mFilename = filename;

        if (mContext->mMode == SM_READ)
        {
            File file;
            file.Open(filename, FF_SHARE_READ, FILE_OPEN_EXISTING);
            if (file.IsOpen())
            {
                String text;
                text.Resize(static_cast<size_t>(file.GetSize()));
                file.Read(const_cast<Char8*>(text.CStr()), text.Size());
                file.Close();
                ReadAllText(text);
            }
        }
        mContext->mModeStack.push(PM_NONE);
    }
    void TextStream::Close()
    {
        if (!mContext)
        {
            return;
        }
        if (!IsReading())
        {
            File file;
            String text;
            String& outputText = mContext->mOutputText ? *mContext->mOutputText : text;
            WriteAllText(outputText);
            file.Open(mContext->mFilename, FF_WRITE, FILE_OPEN_ALWAYS);
            if (file.IsOpen())
            {
                file.Write(outputText.CStr(), outputText.Size());
                file.Close();
            }
        }
        Clear();
        mContext->~TextStreamContext();
        FreeContext(reinterpret_cast<StreamContext*&>(mContext));
    }

    void TextStream::Clear()
    {
        ReleaseContext();
    }

    void TextStream::Reset(Stream::StreamMode mode)
    {
        if (mContext)
        {
            mContext->mMode = mode;
            while (!mContext->mModeStack.empty())
            {
                mContext->mModeStack.pop();
            }
            while (!mContext->mPropertyInfos.empty())
            {
                mContext->mPropertyInfos.pop();
            }
            mContext->mBoundObject = NULL_PTR;
            mContext->mModeStack.push(PM_NONE);
        }
    }

    // Normal Value Serialization
    void TextStream::Serialize(UInt8& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    value = static_cast<UInt8>(ToUInt32(valueString));
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(UInt16& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    value = static_cast<UInt16>(ToUInt32(valueString));
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(UInt32& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    value = static_cast<UInt32>(ToUInt32(valueString));
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(UInt64& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    value = ToUInt64(valueString);
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(Int8& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    value = static_cast<Int8>(ToInt32(valueString));
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(Int16& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    value = static_cast<Int16>(ToInt32(valueString));
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(Int32& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    value = static_cast<Int32>(ToInt32(valueString));
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(Int64& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    value = ToInt64(valueString);
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(Float32& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    value = static_cast<Float32>(ToFloat32(valueString));
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value, FLOAT_PRECISION));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(Float64& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    value = ToFloat32(valueString);
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(static_cast<Float32>(value), FLOAT_PRECISION));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(Vector2& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    ToVector2(valueString, value);
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value, FLOAT_PRECISION));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(Vector3& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    ToVector3(valueString, value);
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value, FLOAT_PRECISION));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(Vector4& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    ToVector4(valueString, value);
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value, FLOAT_PRECISION));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(Color& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    ToColor(valueString, value);
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value, FLOAT_PRECISION));
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(String& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, value))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, String("\"") + value + "\"");
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(Token& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    value = Token(valueString);
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, String("\"") + value.CStr() + "\"");
            }
            mContext->mPropertyInfos.pop();
        }
    }
    void TextStream::Serialize(const Type*& value)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (IsReading())
        {
            Token typeName;
            Serialize(typeName);
            if (typeName.Empty())
            {
                value = nullptr;
            }
            else
            {
                value = InternalHooks::gFindType(typeName);
            }
        }
        else
        {
            // todo: const_cast and ref hack would be slightly faster...
            Token typeName = value ? value->GetFullName() : Token();
            Serialize(typeName);
        }
    }

    void TextStream::SerializeGuid(ByteT* value, SizeT size)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
        }
        else
        {
            if (IsReading())
            {
                String valueString;
                if (!FindBoundPropertyValue(GetCurrentPropertyInfo().name, valueString))
                {
                    ErrorPropertyNotFound(GetCurrentPropertyInfo().name);
                }
                else
                {
                    ToGuid(valueString, value, size);
                }
            }
            else
            {
                PushProperty(GetCurrentPropertyInfo().name, ToString(value, size));
            }
            mContext->mPropertyInfos.pop();
        }
    }

    void TextStream::SerializeAsset(Token& value, bool)
    {
        Serialize(value);
    }


    void TextStream::Serialize(const StreamPropertyInfo& info)
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        // Must call open first!
        Assert(mContext);
        mContext->mPropertyInfos.push(info);
    }

    bool TextStream::BeginObject(const String& name, const String& super)
    {
        // Must call open first!
        Assert(mContext);
        if (CheckParseMode(PM_NONE))
        {
            ErrorInvalidSerializationState(mContext->mModeStack.empty() ? INVALID : mContext->mModeStack.top());
            return false;
        }
        mContext->mModeStack.push(PM_OBJECT);
        if (IsReading())
        {
            mContext->mBoundObject = FindObject(name);
            if (!mContext->mBoundObject)
            {
                ErrorMissingObject(name);
                mContext->mModeStack.pop();
                return false;
            }
        }
        else
        {
            AddStreamObject(name, super, true);
        }
        return true;
    }
    void TextStream::EndObject()
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        if (CheckParseMode(PM_OBJECT))
        {
            ErrorInvalidSerializationState(mContext->mModeStack.empty() ? INVALID : mContext->mModeStack.top());
            return;
        }
        PopObject();
    }

    bool TextStream::BeginStruct()
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return false;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
            return false;
        }
        const String& name = GetCurrentPropertyInfo().name;
        if (IsReading())
        {
            StreamPropertyWPtr property = mContext->mBoundObject->FindBoundProperty(name);
            if (!property)
            {
                property = mContext->mBoundObject->FindProperty(name);
            }
            if (!property)
            {
                ErrorPropertyNotFound(name);
                return false;
            }
            if (property->type != StreamPropertyType::SPT_STRUCT)
            {
                ErrorInvalidPropertyType(name, TStreamPropertyType::GetString(property->type));
                return false;
            }
            mContext->mBoundObject->BindProperty(property);
            mContext->mModeStack.push(PM_STRUCT);
        }
        else
        {
            PushStruct(name);
        }
        return true;
    }
    void TextStream::EndStruct()
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        PopStruct();
        mContext->mPropertyInfos.pop();
    }

    bool TextStream::BeginArray()
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return false;
        }

        if (!HasPropertyInfo())
        {
            ErrorMissingPropertyName();
            return false;
        }
        const String& name = GetCurrentPropertyInfo().name;
        if (IsReading())
        {
            StreamPropertyWPtr property = mContext->mBoundObject->FindBoundProperty(name);
            if (!property)
            {
                property = mContext->mBoundObject->FindProperty(name);
            }
            if (!property)
            {
                ErrorPropertyNotFound(name);
                return false;
            }
            if (property->type != StreamPropertyType::SPT_ARRAY)
            {
                ErrorInvalidPropertyType(name, TStreamPropertyType::GetString(property->type));
                return false;
            }
            mContext->mBoundObject->BindProperty(property);
            if (CheckParseMode(PM_ARRAY))
            {
                mContext->mModeStack.push(PM_ARRAY);
            }
        }
        else
        {
            PushArray(name);
        }
        return true;
    }
    void TextStream::EndArray()
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return;
        }

        // Property doesn't exist.
        if (!mContext->mBoundObject->GetBoundProperty())
        {
            Assert(IsReading());
            return;
        }

        // Must call Open first!
        Assert(mContext);
        PopArray();
        mContext->mPropertyInfos.pop();
    }

    size_t TextStream::GetArraySize() const
    {
        // Text we've parsed does not contain any objects. Abort all operations early.
        if (!mContext->mBoundObject)
        {
            return 0;
        }

        // Property doesn't exist.
        if (!(mContext->mBoundObject->GetBoundProperty()))
        {
            Assert(IsReading());
            return 0;
        }

        // These should never trigger!
        Assert(IsReading() && TopIsArray());
        return mContext->mBoundObject->GetBoundProperty()->children.Size();
    }

    void TextStream::SetArraySize(size_t)
    {
        // TArray size is implicit, set when we parse (reading) or writing objects (writing)
    }

    StreamContext* TextStream::PopContext()
    {
        StreamContext* context = mContext;
        mContext = nullptr;
        return context;
    }

    const StreamContext* TextStream::GetContext() const
    {
        return mContext;
    }

    void TextStream::SetContext(StreamContext* context)
    {
        if (context && context->mType == StreamContext::TEXT)
        {
            if (mContext)
            {
                ReleaseContext();
                mContext->~TextStreamContext();
                FreeContext(reinterpret_cast<StreamContext*&>(mContext));
            }
            mContext = static_cast<TextStreamContext*>(context);
        }
    }

    void TextStream::ReadAllText(const String& text)
    {
        // Must call open!
        Assert(mContext);
        String line;
        size_t cursor = 0;
        size_t lineNum = 0;
        mContext->mModeStack.push(PM_NONE);
        do
        {
            InternalReadLine(cursor, text, line);
            if (line.Empty())
            {
                continue;
            }
            cursor += line.Size() + 1;
            String stripped;
            InternalStrip(line, stripped);
            t_TokenList tokens;
            InternalTokenize(tokens, stripped);
            InternalParse(lineNum, tokens);
            ++lineNum;
        } while (!line.Empty() && cursor < text.Size());
        mContext->mModeStack.pop();
    }

    void TextStream::WriteAllText(String& output)
    {
        // Must call open!
        Assert(mContext);
        static const size_t MAX_PROPERTY = INT_MAX;
        mContext->mModeStack.push(PM_NONE);
        for (size_t i = 0; i < mContext->mObjects.Size(); ++i)
        {
            const StreamObjectWPtr& object = mContext->mObjects[i];
            output += String(TOK_BEGIN_OBJECT) + object->GetType() + String(TOK_PROPERTY_SEPARATOR) + object->GetSuper() + "\n";
            output += "{\n";
            mContext->mModeStack.push(PM_OBJECT);

            size_t propIndex = 0;
            size_t indent = 4;
            while (Valid(propIndex) && object->BindProperty(propIndex))
            {
                const StreamPropertyWPtr& property = object->GetBoundProperty();
                InternalWriteProperty(indent, output, property);
                ++propIndex;
            }

            mContext->mModeStack.pop();
            output += "}\n";
        }
        mContext->mModeStack.pop();
    }

    struct FindObjectByNamePred
    {
        FindObjectByNamePred(const String& inName) : name(inName) {}

        bool operator()(const StreamObjectWPtr& obj) const
        {
            return obj->GetType() == name;
        }
        String name;
    };

    StreamObjectWPtr TextStream::FindObject(const String& name) const
    {
        if (mContext)
        {
            auto it = std::find_if(mContext->mObjects.begin(), mContext->mObjects.end(), FindObjectByNamePred(name));
            if (it != mContext->mObjects.end())
            {
                return *it;
            }
        }
        return NULL_PTR;
    }

    void TextStream::DeleteObject(const String& name) const
    {
        if (mContext)
        {
            auto it = std::find_if(mContext->mObjects.begin(), mContext->mObjects.end(), FindObjectByNamePred(name));
            if (it != mContext->mObjects.end())
            {
                mContext->mObjects.Remove(it);
            }
        }
    }

    size_t TextStream::GetObjectCount() const
    {
        return mContext ? mContext->mObjects.Size() : 0;
    }
    const String& TextStream::GetObjectName(const size_t index) const
    {
        return mContext ? mContext->mObjects[index]->GetType() : EMPTY_STRING;
    }
    const String& TextStream::GetObjectSuper(const size_t index) const
    {
        return mContext ? mContext->mObjects[index]->GetSuper() : EMPTY_STRING;
    }

    String TextStream::GetFilename() const
    {
        if (mContext)
        {
            return mContext->mFilename;
        }
        return EMPTY_STRING;
    }

    void TextStream::InternalReadLine(size_t start, const String& text, String& line)
    {
        size_t lineEnd = text.Size() - 1;
        for (size_t i = start; i < text.Size(); ++i)
        {
            String::value_type c = text[i];
            if (c == '\n')
            {
                text.SubString(start, i - start, line);
                break;
            }
            else if (i == lineEnd)
            {
                text.SubString(start, (i)-start, line);
            }
        }
    }

    void TextStream::InternalStrip(const String& inLine, String& strippedLine)
    {
        strippedLine = StrStripWhitespace(inLine, true);

        // Remove EndLine:

        if (!strippedLine.Empty())
        {
            SizeT cursor = strippedLine.Size() - 1;
            if (strippedLine[cursor] == '\n')
            {
                --cursor;
                if (Invalid(cursor))
                {
                    strippedLine = EMPTY_STRING;
                    return;
                }
            }
            if (strippedLine[cursor] == '\r')
            {
                --cursor;
                if (Invalid(cursor))
                {
                    strippedLine = EMPTY_STRING;
                    return;
                }
            }

            String sub;
            strippedLine.SubString(0, cursor + 1, sub);
            strippedLine = std::move(sub);
        }
    }

    void TextStream::InternalTokenize(t_TokenList& tokens, const String& line)
    {
        // Expects no whitespace, or endline.

        // Should spit out 1,2, or 3 tokens.
        // Examples
        // tokenize("$Example=Base")
        // tokenize("Prop=Value")
        // tokenize("{")

        const char* lineCStr = line.CStr();
        size_t lineSize = line.Size();
        size_t lineEnd = lineSize - 1;
        size_t cursor = 0;


        for (size_t i = 0; i < lineSize; ++i)
        {
            String::value_type c = lineCStr[i];
            String::value_type pc = i > 0 ? lineCStr[i - 1] : INVALID8;
            // $ or @
            if (c == TOK_BEGIN_OBJECT || c == TOK_STREAM_VAR)
            {
                tokens.Add(String(c));
                cursor = i + 1;
            }
            // },
            else if (pc == TOK_END_STRUCT)
            {
                tokens.Add(String(TOK_END_STRUCT));
            }
            // take substr of before =
            else if (c == TOK_PROPERTY_SEPARATOR)
            {
                String sub;
                line.SubString(cursor, i - cursor, sub);
                tokens.Add(std::move(sub));
                cursor = i + 1;
            }
            // take substr after =
            else if (i == lineEnd)
            {
                String sub;
                line.SubString(cursor, sub);
                tokens.Add(std::move(sub));
            }
        }

        // Verify Token List
        for (size_t i = 0; i < tokens.Size(); ++i)
        {
            // Remove surrounding quotes: Should only be affecting the property types
            String& token = tokens[i];
            SizeT firstQuote = token.Find('\"');
            SizeT lastQuote = token.FindLast('\"');
            if (firstQuote != lastQuote && Valid(firstQuote) && Valid(lastQuote))
            {
                String sub;
                token.SubString(firstQuote + 1, lastQuote - (firstQuote + 1), sub);
                token = std::move(sub);
            }
        }
    }
    void TextStream::InternalParse(size_t line, t_TokenList& tokens)
    {
        Assert(!mContext->mModeStack.empty());
        ParseMode currentMode = mContext->mModeStack.top();

        // eg. $Type=Base
        //     @Var=Value
        if (tokens.Size() == 3)
        {
            String::value_type tokenChar = tokens[0][0];
            if (currentMode == PM_NONE)
            {
                if (tokenChar == TOK_BEGIN_OBJECT)
                {
                    AddStreamObject(tokens[1], tokens[2], true);
                }
                else if (tokenChar == TOK_STREAM_VAR)
                {
                    AddStreamVariable(tokens[1], tokens[2]);
                }
                else
                {
                    ErrorUnsupportedToken(line, String(tokenChar));
                }
            }
            else
            {
                if (tokenChar == TOK_BEGIN_OBJECT)
                {
                    ErrorInvalidTokenState(line, tokenChar, currentMode);
                }
                else if (tokenChar == TOK_STREAM_VAR)
                {
                    AddStreamVariable(tokens[1], tokens[2]);
                }
                else
                {
                    ErrorUnsupportedToken(line, String(tokenChar));
                }
            }
        }
        // eg. Name=Value
        //     Name={
        //     Name=[
        //     },
        else if (tokens.Size() == 2)
        {
            if (currentMode == PM_NONE)
            {
                ErrorInvalidState(line, currentMode);
            }
            else
            {
                if (tokens[1][0] == TOK_BEGIN_ARRAY)
                {
                    // PushArray(name, unknown)
                    PushArray(tokens[0]);
                }
                else if (tokens[1][0] == TOK_BEGIN_STRUCT)
                {
                    // PushStruct(name)
                    PushStruct(tokens[0]);
                }
                else
                {
                    // PushProperty(name,value)
                    PushProperty(tokens[0], tokens[1]);
                }
            }
        }
        // eg. {
        //     } 
        else if (tokens.Size() == 1)
        {
            String::value_type tokenChar = tokens[0][0];
            if (tokenChar == TOK_BEGIN_STRUCT)
            {
                // Push only while in array mode.. AddObject uses same token.
                if (currentMode == PM_ARRAY)
                {
                    PushStruct(EMPTY_STRING);
                }
                else if (currentMode == PM_NONE)
                {
                    if (mContext->mBoundObject == NULL_PTR)
                    {
                        ErrorMissingObject(line);
                    }
                    mContext->mModeStack.push(PM_OBJECT);
                }
                else
                {
                    ErrorInvalidState(line, currentMode);
                }
            }
            else if (tokenChar == TOK_END_STRUCT)
            {
                // PopStruct()
                if (currentMode == PM_STRUCT)
                {
                    PopStruct();
                }
                else if (currentMode == PM_OBJECT)
                {
                    PopObject();
                }
                else
                {
                    ErrorInvalidState(line, currentMode);
                }
            }
            else if (tokenChar == TOK_END_ARRAY)
            {
                // PopArray
                PopArray();
                if (currentMode != PM_ARRAY)
                {
                    ErrorInvalidState(line, currentMode);
                }
            }
            else if (currentMode == PM_ARRAY)
            {
                PushProperty(EMPTY_STRING, tokens[0]);
            }
            else
            {
                ErrorUnsupportedToken(line, String(tokenChar));
            }
        }
        else
        {
            ErrorUnexpectedTokenCount(line, tokens.Size());
        }
    }

    void TextStream::InternalWriteProperty(size_t space, String& text, const StreamPropertyWPtr& property)
    {
        switch (property->type)
        {
            case StreamPropertyType::SPT_NORMAL:
                InternalAddWhitespace(space, text);
                if (mContext->mModeStack.top() == PM_ARRAY)
                {
                    text += property->valueString + "\n";
                }
                else
                {
                    text += property->name + String(TOK_PROPERTY_SEPARATOR) + property->valueString + "\n";
                }
                break;
            case StreamPropertyType::SPT_STRUCT:
                InternalAddWhitespace(space, text);
                if (mContext->mModeStack.top() == PM_ARRAY)
                {
                    text += String(TOK_BEGIN_STRUCT) + "\n";
                }
                else
                {
                    text += property->name + String(TOK_PROPERTY_SEPARATOR) + String(TOK_BEGIN_STRUCT) += "\n";
                }
                space += 4;
                mContext->mModeStack.push(PM_STRUCT);
                for (size_t i = 0; i < property->children.Size(); ++i)
                {
                    InternalWriteProperty(space, text, property->children[i]);
                }
                mContext->mModeStack.pop();
                space -= 4;
                InternalAddWhitespace(space, text);
                text += String(TOK_END_STRUCT) + "\n";
                break;
            case StreamPropertyType::SPT_ARRAY:
                InternalAddWhitespace(space, text);
                text += property->name + String(TOK_PROPERTY_SEPARATOR) + String(TOK_BEGIN_ARRAY) + "\n";
                space += 4;
                mContext->mModeStack.push(PM_ARRAY);
                for (size_t i = 0; i < property->children.Size(); ++i)
                {
                    InternalWriteProperty(space, text, property->children[i]);
                }
                mContext->mModeStack.pop();
                space -= 4;
                InternalAddWhitespace(space, text);
                text += String(TOK_END_ARRAY) + "\n";
                break;
        }
    }

    void TextStream::InternalAddWhitespace(size_t space, String& text)
    {
        text.Reserve(space + text.Size());
        for (size_t i = 0; i < space; ++i)
        {
            text.Append(' ');
        }
    }

    bool TextStream::TopIsArray() const
    {
        if (!mContext || !mContext->mBoundObject || !mContext->mBoundObject->GetBoundProperty())
        {
            return false;
        }
        if (mContext->mBoundObject->GetBoundProperty()->type == StreamPropertyType::SPT_ARRAY &&
            mContext->mModeStack.top() == PM_ARRAY)
        {
            return true;
        }
        return false;
    }
    bool TextStream::TopIsStruct() const
    {
        if (!mContext || !mContext->mBoundObject || mContext->mBoundObject->GetBoundProperty())
        {
            return false;
        }
        if (mContext->mBoundObject->GetBoundProperty()->type == StreamPropertyType::SPT_STRUCT &&
            mContext->mModeStack.top() == PM_STRUCT)
        {
            return true;
        }
        return false;
    }

    bool TextStream::HasPropertyInfo()
    {
        return mContext && !mContext->mPropertyInfos.empty() && !mContext->mPropertyInfos.top().name.Empty();
    }

    bool TextStream::CheckParseMode(ParseMode mode) const
    {
        if (mContext->mModeStack.empty() || mContext->mModeStack.top() != mode)
        {
            return true;
        }
        return false;
    }

    const StreamPropertyInfo& TextStream::GetCurrentPropertyInfo()
    {
        return mContext->mPropertyInfos.top();
    }

    void TextStream::AddStreamObject(const String& type, const String& super, bool bind)
    {
        StreamObjectPtr object(LFNew<StreamObject>());
        object->SetType(type);
        object->SetSuper(super);
        object->SetSelf(object);
        mContext->mObjects.Add(object);
        if (bind)
        {
            mContext->mBoundObject = object;
        }
    }

    struct FindVarByName
    {
        FindVarByName(const String& inName) : name(inName) {}

        bool operator()(const StreamVariable& var) const
        {
            return var.name == name;
        }
        String name;
    };

    void TextStream::AddStreamVariable(const String& name, const String& value)
    {
        auto it = std::find_if(mContext->mVariables.begin(), mContext->mVariables.end(), FindVarByName(name));
        if (it == mContext->mVariables.end())
        {
            StreamVariable var;
            var.name = name;
            var.valueString = value;
            mContext->mVariables.Add(var);
        }
        else
        {
            it->valueString = value;
        }
    }

    void TextStream::PushStruct(const String& name)
    {
        mContext->mModeStack.push(PM_STRUCT);

        // Support Context Saving by reading and binding existing properties.
        StreamPropertyWPtr existingProp = mContext->mBoundObject->FindBoundProperty(name);
        if (!existingProp)
        {
            existingProp = mContext->mBoundObject->FindProperty(name);
        }
        if (existingProp)
        {
            Assert(existingProp->type == StreamPropertyType::SPT_STRUCT);
            mContext->mBoundObject->BindProperty(existingProp);
            return;
        }

        StreamPropertyPtr property(LFNew<StreamProperty>());
        // If empty, were apart of an array.. Although names shouldn't matter we'll add for debugging for now.
        if (name == EMPTY_STRING)
        {
            property->name = ToString(mContext->mBoundObject->GetBoundProperty()->children.Size());
        }
        else
        {
            property->name = name;
        }

        property->type = StreamPropertyType::SPT_STRUCT;
        mContext->mBoundObject->AddProperty(property);
        mContext->mBoundObject->BindProperty(StaticCast<StreamPropertyWPtr>(property)); // Be explicit
    }
    void TextStream::PushArray(const String& name)
    {
        // TArray of TArray is not allowed!
        if (mContext->mModeStack.top() != PM_ARRAY)
        {
            mContext->mModeStack.push(PM_ARRAY);
        }

        StreamPropertyWPtr existingProp = mContext->mBoundObject->FindBoundProperty(name);
        if (!existingProp)
        {
            existingProp = mContext->mBoundObject->FindProperty(name);
        }
        if (existingProp)
        {
            Assert(existingProp->type == StreamPropertyType::SPT_ARRAY);
            mContext->mBoundObject->BindProperty(existingProp);
            return;
        }

        StreamPropertyPtr property(LFNew<StreamProperty>());
        property->name = name;
        property->type = StreamPropertyType::SPT_ARRAY;
        mContext->mBoundObject->AddProperty(property);
        mContext->mBoundObject->BindProperty(StaticCast<StreamPropertyWPtr>(property)); // Be explicit
    }
    void TextStream::PushProperty(const String& name, const String& value)
    {
        StreamPropertyWPtr existingProp = mContext->mBoundObject->FindBoundProperty(name);
        if (!existingProp)
        {
            existingProp = mContext->mBoundObject->FindProperty(name);
        }
        if (existingProp)
        {
            Assert(existingProp->type == StreamPropertyType::SPT_NORMAL);
            existingProp->valueString = value;
            return;
        }

        StreamPropertyPtr property(LFNew<StreamProperty>());
        // If empty, were apart of an array.. Although names shouldn't matter we'll add for debugging for now.
        if (name == EMPTY_STRING)
        {
            property->name = ToString(mContext->mBoundObject->GetBoundProperty()->children.Size());
        }
        else
        {
            property->name = name;
        }
        property->type = StreamPropertyType::SPT_NORMAL;
        property->valueString = value;
        mContext->mBoundObject->AddProperty(property);
    }

    void TextStream::PopStruct()
    {
        // eg. TArray > Struct > TArray > Struct
        // would bounce out to TArray so then we can go back to struct if necessary
        StreamPropertyWPtr property = mContext->mBoundObject->GetBoundProperty();
        mContext->mBoundObject->BindProperty(property->parent);
        mContext->mModeStack.pop();
    }
    void TextStream::PopArray()
    {
        StreamPropertyPtr property = mContext->mBoundObject->GetBoundProperty();
        mContext->mBoundObject->BindProperty(property->parent);
        mContext->mModeStack.pop();
    }

    void TextStream::PopObject()
    {
        mContext->mBoundObject = NULL_PTR;
        mContext->mModeStack.pop();
    }

    void TextStream::ReleaseContext()
    {
        if (!mContext)
        {
            return;
        }
        while (!mContext->mModeStack.empty())
        {
            mContext->mModeStack.pop();
        }
        while (!mContext->mPropertyInfos.empty())
        {
            mContext->mPropertyInfos.pop();
        }
        mContext->mFilename.Clear();
        mContext->mBoundObject = NULL_PTR;
        mContext->mObjects.Clear();
        mContext->mVariables.Clear();
        mContext->mOutputText = nullptr;
    }

    bool TextStream::FindBoundPropertyValue(const String& name, String& outValueString)
    {
        if (!mContext->mBoundObject)
        {
            return false;
        }
        StreamPropertyWPtr prop = mContext->mBoundObject->FindBoundProperty(name);
        if (!prop)
        {
            prop = mContext->mBoundObject->FindProperty(name);
        }
        if (!prop)
        {
            return false;
        }
        outValueString = prop->valueString;
        return true;
    }

    void TextStream::ErrorMissingObject(size_t line)
    {
        gSysLog.Error(LogMessage("Stream Error: (") << line << "): Missing Object.");
    }

    void TextStream::ErrorMissingObject(const String& name)
    {
        if (mContext->mLogWarnings)
        {
            
            gSysLog.Warning(LogMessage("Stream Error: Missing Object ") << name);
        }
    }

    void TextStream::ErrorMissingPropertyName()
    {
        gSysLog.Error(LogMessage("Stream Error: Missing property name."));
    }

    void TextStream::ErrorPropertyNotFound(const String& name)
    {
        if (!mContext->mLogWarnings)
        {
            return;
        }
        // todo: Need the filepath runtime.
        // io::DomainId domain = io::GetDomainId(mContext->filename);
        // String path = mContext->mFilename;
        // if (domain != io::INVALID_DOMAIN)
        // {
        //     DebugAssert(io::StripDomainPath(domain, path) == io::IOR_NO_ERROR);
        // }
        String path = "{Missing failpath runtime. TextStream.cpp}";
        gSysLog.Warning(LogMessage("Stream Error: Property not found ") << name << " in " << path);
    }

    void TextStream::ErrorUnsupportedToken(size_t line, const String& token)
    {
        gSysLog.Error(LogMessage("Stream Error: (") << line << "): Unsupported Token " << token);
    }
    void TextStream::ErrorInvalidTokenState(size_t line, String::value_type token, size_t state)
    {
        gSysLog.Error(LogMessage("Stream Error: (") << line << "): Invalid Token State state=" << state << ", token=" << token);
    }
    void TextStream::ErrorInvalidState(size_t line, size_t state)
    {
        gSysLog.Error(LogMessage("Stream Error: (") << line << "): Invalid Parse State" << state);
    }

    void TextStream::ErrorInvalidPropertyType(const String& name, const String& type)
    {
        gSysLog.Error(LogMessage("Stream Error: Invalid property type, property=") << name << ", type=" << type);
    }

    void TextStream::ErrorUnexpectedTokenCount(size_t line, size_t tokenCount)
    {
        gSysLog.Error(LogMessage("Stream Error: (") << line << "): Unexpected Token Count " << tokenCount);
    }

    void TextStream::ErrorInvalidSerializationState(size_t state)
    {
        gSysLog.Error(LogMessage("Stream Error: Invalid serialization state (") << state << ")");
    }

}
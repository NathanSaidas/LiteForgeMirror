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

// #include "core/io/Stream.h"
// #include "core/io/IOTypes.h"
// #include "core/system/Enum.h"
// #include "core/system/SmartPointer.h"
// #include "core/util/Array.h"

#include "Core/IO/Stream.h"
#include "Core/Common/Enum.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Utility/Array.h"

#include <stack>

namespace lf
{
    DECLARE_STRICT_ENUM(StreamPropertyType,
        SPT_NORMAL,
        SPT_STRUCT,
        SPT_ARRAY
        );
    class StreamObject;
    class StreamProperty;
    using StreamPropertyWPtr = TWeakPointer<StreamProperty>;
    using StreamPropertyPtr = TStrongPointer<StreamProperty>;
    using StreamObjectWPtr = TWeakPointer<StreamObject>;
    using StreamObjectPtr = TStrongPointer<StreamObject>;

    using StreamPropertyList = TVector<StreamPropertyPtr>;

    struct LF_CORE_API StreamVariable
    {
    public:
        String name;
        String valueString;
    };

    class LF_CORE_API StreamProperty
    {
    public:
        StreamPropertyType type;
        String name;
        String valueString;
        StreamPropertyList children;
        StreamPropertyWPtr parent;
        StreamObjectWPtr context;
    };

    class LF_CORE_API StreamObject
    {
    public:
        StreamObject();
        ~StreamObject();

        void AddProperty(const StreamPropertyPtr& property);
        void RemoveProperty(const StreamPropertyPtr& property);
        void RemoveProperty(const String& name);
        void Clear();

        StreamPropertyWPtr FindProperty(const String& name);
        StreamPropertyWPtr FindBoundProperty(const String& name);

        bool BindProperty(size_t index);
        void BindProperty(const StreamPropertyWPtr& property);
        void Unbind();

        LF_INLINE void SetType(const String& type) { mType = type; }
        LF_INLINE void SetSuper(const String& super) { mSuper = super; }
        LF_INLINE const String& GetType() const { return mType; }
        LF_INLINE const String& GetSuper() const { return mSuper; }

        LF_INLINE const StreamPropertyList& GetProperties() const { return mProperties; }
        LF_INLINE StreamPropertyWPtr GetBoundProperty() const { return mBoundProperty; }

        LF_INLINE void SetSelf(const StreamObjectWPtr& self) { mSelf = self; }

    private:
        String mType;
        String mSuper;
        StreamPropertyList mProperties;
        StreamPropertyWPtr mBoundProperty;
        StreamObjectWPtr mSelf;
    };

    // $Example=Format
    // {
    //     Name=Value
    //     Struct={
    //         Prop1=Value
    //         Prop2=Value
    //     }
    //     Array=[
    //         Value
    //         Value
    //     ]
    //     StructArray=[
    //         {
    //             Prop1=Value
    //         }
    //         {
    //             Prop1=Value
    //         }
    //     ]
    // }
    // @version=32
    // @encrypt=on
    // @encrypt=off
    // @default_base=ShieldHeart::Object 

    // $ = StreamObject
    // "Name=Value" = StreamProperty
    // "Name={" = StreamStruct
    // "Name=[" = StreamArray

    typedef TVector<StreamObjectPtr> StreamObjectList;
    typedef TVector<StreamVariable> StreamVariableList;

    class LF_CORE_API TextStream : public Stream
    {
    public:
        typedef TVector<String> t_TokenList;

        TextStream();
        TextStream(Stream::StreamText, String* text, StreamMode mode);
        TextStream(Stream::StreamFile, const String& filename, StreamMode mode);
        TextStream(StreamContext*& context);
        ~TextStream();

        // **********************************
        // Opens the stream with the given mode for text.
        // When {mode} is SM_READ the text will be parsed into internal objects.
        // When {mode} is SM_WRITE text will be generated from internal objects upon {Close} being called.
        // Fails if the stream is already open or there is an invalid argument.
        // **********************************
        void Open(Stream::StreamText, String* text, StreamMode mode);
        // **********************************
        // Opens the stream with the given mode for a file.
        // When {mode} is SM_READ the file will be parsed into internal objects.
        // When {mode} is SM_WRITE text will be generated and written to file from internal objects upon {Close} being called.
        // Fails if the stream is already open or there is an invalid argument.
        // **********************************
        void Open(Stream::StreamFile, const String& filename, StreamMode mode);
        // **********************************
        // Closes the stream releasing used resources.
        // When {mode} is SM_WRITE content is written to out the output target.
        // **********************************
        virtual void Close() override;
        // **********************************
        // Clears all content from the stream and resets it, in its current mode.
        // **********************************
        virtual void Clear() override;
        // **********************************
        // Changes the stream mode and resets the internal cursor for data serialization.
        // This is useful if you're writing a "template" object and then having other instances
        // read and copy from it.
        // **********************************
        void Reset(Stream::StreamMode mode);

        // Normal Value Serialization
        virtual void Serialize(UInt8& value) override;
        virtual void Serialize(UInt16& value) override;
        virtual void Serialize(UInt32& value) override;
        virtual void Serialize(UInt64& value) override;
        virtual void Serialize(Int8& value) override;
        virtual void Serialize(Int16& value) override;
        virtual void Serialize(Int32& value) override;
        virtual void Serialize(Int64& value) override;
        virtual void Serialize(Float32& value) override;
        virtual void Serialize(Float64& value) override;
        virtual void Serialize(Vector2& value) override;
        virtual void Serialize(Vector3& value) override;
        virtual void Serialize(Vector4& value) override;
        virtual void Serialize(Color& value) override;
        virtual void Serialize(String& value) override;
        virtual void Serialize(Token& value) override;
        virtual void Serialize(const Type*& value) override;
        virtual void SerializeGuid(ByteT* value, SizeT size) override;
        virtual void SerializeAsset(Token& value, bool isWeak) override;
        virtual void Serialize(const StreamPropertyInfo& info) override;
        virtual void Serialize(const ArrayPropertyInfo& info) override;
        void Serialize(MemoryBuffer& value) override;

        virtual bool BeginObject(const String& name, const String& super) override;
        virtual void EndObject() override;

        virtual bool BeginStruct() override;
        virtual void EndStruct() override;

        virtual bool BeginArray() override;
        virtual void EndArray() override;
        virtual size_t GetArraySize() const override;
        virtual void SetArraySize(size_t size) override;

        

        void ReadAllText(const String& text);
        void WriteAllText(String& output);

        StreamObjectWPtr FindObject(const String& name) const;
        void DeleteObject(const String& name) const;

        size_t GetObjectCount() const override;
        const String& GetObjectName(const size_t index) const override;
        const String& GetObjectSuper(const size_t index) const override;

        String GetFilename() const;
    private:
        enum ParseMode
        {
            PM_NONE,
            PM_OBJECT,
            PM_STRUCT,
            PM_ARRAY
        };

        typedef std::stack<ParseMode> ParseModeStack;
        typedef std::stack<StreamPropertyInfo> PropertyInfoStack;

        class TextStreamContext : public StreamContext
        {
        public:
            StreamObjectList mObjects;
            StreamVariableList mVariables;
            ParseModeStack mModeStack;

            StreamObjectWPtr mBoundObject;
            String mFilename;
            String* mOutputText;

            PropertyInfoStack mPropertyInfos;
        };

        virtual StreamContext* PopContext() override;
        virtual const StreamContext* GetContext() const override;
        virtual void SetContext(StreamContext* context) override;

        void InternalReadLine(size_t start, const String& text, String& line);
        void InternalStrip(const String& inLine, String& strippedLine);
        void InternalTokenize(t_TokenList& tokens, const String& line);
        void InternalParse(size_t line, t_TokenList& tokens);

        void InternalWriteProperty(size_t space, String& text, const StreamPropertyWPtr& property);
        void InternalAddWhitespace(size_t space, String& text);

        bool TopIsArray() const;
        bool TopIsStruct() const;
        bool HasPropertyInfo();
        // Assumes Context!=NULL
        bool CheckParseMode(ParseMode mode) const;
        const StreamPropertyInfo& GetCurrentPropertyInfo();

        void AddStreamObject(const String& type, const String& super, bool bind);
        void AddStreamVariable(const String& name, const String& value);

        void PushStruct(const String& name);
        void PushArray(const String& name);
        void PushProperty(const String& name, const String& value);

        void PopStruct();
        void PopArray();
        void PopObject();

        void ReleaseContext();

        bool FindBoundPropertyValue(const String& name, String& outValueString);

        void ErrorMissingObject(size_t line);
        void ErrorMissingObject(const String& name);
        void ErrorMissingPropertyName();
        void ErrorPropertyNotFound(const String& name);
        void ErrorUnsupportedToken(size_t line, const String& token);
        void ErrorInvalidTokenState(size_t line, String::value_type token, size_t state);
        void ErrorInvalidState(size_t line, size_t state);
        void ErrorInvalidPropertyType(const String& name, const String& type);
        void ErrorUnexpectedTokenCount(size_t line, size_t tokenCount);
        void ErrorInvalidSerializationState(size_t state);

        TextStreamContext* mContext;
    };
}
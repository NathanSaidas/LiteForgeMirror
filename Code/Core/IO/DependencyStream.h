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
#include "Core/IO/Stream.h"

namespace lf {

class LF_CORE_API DependencyStream : public Stream
{
public:
    using CollectionType = TVector<Token>;
    DependencyStream();
    DependencyStream(CollectionType* weakDeps, CollectionType* strongDeps);

    void Close() override;
    void Clear() override;

    void Serialize(bool& value) override;
    void SerializeAsset(Token& value, bool isWeak) override;

    bool BeginObject(const String& name, const String& super) override;
    void EndObject() override;

    bool BeginStruct() override;
    void EndStruct() override;

    bool BeginArray() override;
    void EndArray() override;

    const StreamContext* GetContext() const;

    // Open the stream to write weak/strong deps to collections
    void Open(CollectionType* weakDeps, CollectionType* strongDeps);

private:
    struct DependencyStreamContext : public StreamContext
    {
    public:
        DependencyStreamContext()
        : StreamContext()
        , mWeakDeps(nullptr)
        , mStrongDeps(nullptr)
        {}

        CollectionType* mWeakDeps;
        CollectionType* mStrongDeps;
    };

    DependencyStreamContext mContext;
};

} // namespace lf
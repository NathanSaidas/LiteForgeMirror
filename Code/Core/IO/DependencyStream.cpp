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
#include "Core/PCH.h"
#include "DependencyStream.h"
#include "Core/String/Token.h"

namespace lf {

DependencyStream::DependencyStream() 
: Stream()
, mContext()
{

}
DependencyStream::DependencyStream(CollectionType* weakDeps, CollectionType* strongDeps) 
: Stream()
, mContext()
{
    Open(weakDeps, strongDeps);
}

void DependencyStream::Close()
{
    mContext.mWeakDeps = nullptr;
    mContext.mStrongDeps = nullptr;
}
void DependencyStream::Clear()
{

}

void DependencyStream::Serialize(bool&)
{
    // explicitly do nothing:
}
void DependencyStream::SerializeAsset(Token& value, bool isWeak)
{
    if (value.Empty())
    {
        return;
    }

    if (isWeak)
    {
        // add unique-only
        if (mContext.mWeakDeps && std::find(mContext.mWeakDeps->begin(), mContext.mWeakDeps->end(), value) == mContext.mWeakDeps->end())
        {
            mContext.mWeakDeps->push_back(value);
        }
    }
    else
    {
        if (mContext.mStrongDeps && std::find(mContext.mStrongDeps->begin(), mContext.mStrongDeps->end(), value) == mContext.mStrongDeps->end())
        {
            mContext.mStrongDeps->push_back(value);
        }
    }
}

bool DependencyStream::BeginObject(const String&, const String&)
{
    return(true);
}
void DependencyStream::EndObject()
{

}

bool DependencyStream::BeginStruct()
{
    return(true);
}
void DependencyStream::EndStruct()
{

}

bool DependencyStream::BeginArray()
{
    return(true);
}
void DependencyStream::EndArray()
{

}

const StreamContext* DependencyStream::GetContext() const
{
    return &mContext;
}

void DependencyStream::Open(CollectionType* weakDeps, CollectionType* strongDeps)
{
    mContext.mType = StreamContext::DEPENDENCY;
    mContext.mMode = Stream::SM_WRITE;
    mContext.mWeakDeps = weakDeps;
    mContext.mStrongDeps = strongDeps;
}

} // namespace lf
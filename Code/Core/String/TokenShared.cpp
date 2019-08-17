// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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
#include "Token.h"
#include "TokenTable.h"
#include "Core/Common/Assert.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Memory/Memory.h"
#include "Core/String/StringUtil.h"
#include "Core/String/String.h"

#include <string.h>

namespace lf {

TokenTable* gTokenTable = nullptr;
static const SizeT TOKEN_TABLE_SIZE = 20000;
static const SizeT MAX_TOKEN_SIZE = 0xFFFF;
static UInt32 HashString(const char*& string)
{
    const UInt32 magic = 0x3CD6432D;
    UInt32 seed = 0xC329BCD2;
    while (*string)
    {
        seed *= magic;
        seed ^= static_cast<UInt32>(*string);
        ++string;
    }
    return seed;
}

TokenTable::TokenTable() :
mMap(nullptr),
mMapSize(0),
mLock(),
mShutdown(false)
{

}
TokenTable::~TokenTable()
{
    CriticalAssertEx(mMap == nullptr, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
}

void TokenTable::LookUp(const char* string, Token& token, AcquireTag)
{
    if (mMap == nullptr)
    {
        CriticalAssertMsgEx("Token table not initialized! Use STATIC_TOKEN instead.", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return;
    }
    if (string == gNullString)
    {
        return;
    }

    const char* end = string;
    UInt32 hash = HashString(end);
    UInt32 hashIdx = hash % mMapSize;
    HashKey& key = mMap[hashIdx];

    if ((end - string) >= MAX_TOKEN_SIZE)
    {
        CriticalAssertMsgEx("Token string is too large.", LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
        return;
    }

    ScopedCriticalSection lock(mLock);
    AcquireNode(token, key, string, end, static_cast<UInt16>(hashIdx));
}
void TokenTable::LookUp(const char* string, Token& token, CopyOnWriteTag)
{
    if (mMap == nullptr)
    {
        CriticalAssertMsgEx("Token table not initialized! Use STATIC_TOKEN instead.", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return;
    }
    if (string == gNullString)
    {
        return;
    }

    const char* end = string;
    UInt32 hash = HashString(end);
    UInt32 hashIdx = hash % mMapSize;
    HashKey& key = mMap[hashIdx];

    if ((end - string) > MAX_TOKEN_SIZE)
    {
        CriticalAssertMsgEx("Token string is too large.", LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
        return;
    }

    // Node LookUp:
    ScopedCriticalSection lock(mLock);
    bool found = AcquireNode(token, key, string, end, static_cast<UInt16>(hashIdx));
    if (!found)
    {
        // Allocate node:
        HashNode& node = AllocateNode(key);
        // Initialize node:
        node.mString = const_cast<char*>(string);
        _InterlockedExchange(&node.mRefCount, 1);
        node.mReleaseOnDestroy = false;
        node.mSize = static_cast<UInt16>(end - string);

        token.mSize = node.mSize;
        token.mString = node.mString;
        token.mKey = static_cast<UInt16>(hashIdx);
    }
}
void TokenTable::LookUp(const char* string, Token& token)
{
    if (mMap == nullptr)
    {
        CriticalAssertMsgEx("Token table not initialized! Use STATIC_TOKEN instead.", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return;
    }
    if (string == gNullString)
    {
        return;
    }

    const char* end = string;
    UInt32 hash = HashString(end);
    UInt32 hashIdx = hash % mMapSize;
    HashKey& key = mMap[hashIdx];

    if ((end - string) > MAX_TOKEN_SIZE)
    {
        CriticalAssertMsgEx("Token string is too large.", LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
        return;
    }

    // Node LookUp:
    ScopedCriticalSection lock(mLock);
    bool found = AcquireNode(token, key, string, end, static_cast<UInt16>(hashIdx));
    if (!found)
    {
        // Allocate node:
        HashNode& node = AllocateNode(key);
        // Initialize node:
        const SizeT bufferSize = (end - string) + 1;
        node.mString = static_cast<char*>(LFAlloc(bufferSize, 16));
        memcpy(node.mString, string, bufferSize);
        _InterlockedExchange(&node.mRefCount, 1);
        node.mReleaseOnDestroy = true;
        node.mSize = static_cast<UInt16>(end - string);

        token.mSize = node.mSize;
        token.mString = node.mString;
        token.mKey = static_cast<UInt16>(hashIdx);
    }
}

void TokenTable::IncrementReference(Token& token)
{
    if (mMap == nullptr)
    {
        CriticalAssertMsgEx("Token table not initialized! Use STATIC_TOKEN instead.", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return;
    }
    if (token.mString == gNullString)
    {
        return;
    }
    AssertEx(token.mKey < mMapSize, LF_ERROR_BAD_STATE, ERROR_API_CORE);
    HashKey& key = mMap[token.mKey];
    ScopedCriticalSection lock(mLock);
    HashNode* node = AcquireNode(token, key);
    if (node)
    {
        const long MAX_TOKEN_REF = 0x7FFFFFFF;
        _ReadWriteBarrier();
        AssertEx(node->mRefCount != MAX_TOKEN_REF, LF_ERROR_BAD_STATE, ERROR_API_CORE);
        _InterlockedIncrement(&node->mRefCount);
    }
}
void TokenTable::DecrementReference(Token& token)
{
    if (mMap == nullptr)
    {
        CriticalAssertMsgEx("Token table not initialized! Use STATIC_TOKEN instead.", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return;
    }
    if (token.mString == gNullString)
    {
        return;
    }
    AssertEx(token.mKey < mMapSize, LF_ERROR_BAD_STATE, ERROR_API_CORE);
    HashKey& key = mMap[token.mKey];
    ScopedCriticalSection lock(mLock);
    HashNode* node = AcquireNode(token, key);
    if (node)
    {
        _ReadWriteBarrier();
        AssertEx(node->mRefCount > 0, LF_ERROR_BAD_STATE, ERROR_API_CORE);
        if (_InterlockedDecrement(&node->mRefCount) == 0)
        {
            // Deallocate:
            if (node->mReleaseOnDestroy)
            {
                LFFree(node->mString);
            }

            if (key.UsePrimary())
            {
                node->mRefCount = 0;
                node->mReleaseOnDestroy = 0;
                node->mSize = 0;
                node->mString = nullptr;
            }
            else
            {

                const SizeT oldSize = key.GetSize();
                const SizeT newSize = oldSize - 1;

                if (newSize == 1)
                {
                    AssertEx(oldSize == 2, LF_ERROR_BAD_STATE, ERROR_API_CORE);
                    if (node->mString == key.mList[0].mString)
                    {
                        node = &key.mList[1];
                    }
                    else
                    {
                        node = &key.mList[0];
                    }
                    HashNode& primary = key.mPrimary;
                    primary.mString = node->mString;
                    primary.mSize = node->mSize;
                    primary.mReleaseOnDestroy = node->mReleaseOnDestroy;
                    primary.mRefCount = node->mRefCount;
                    node = nullptr;

                    LFFree(key.mList);
                    key.mList = nullptr;
                }
                else
                {
                    // Swap Erase:
                    HashNode* last = &(key.mList[newSize]);
                    node->mString = last->mString;
                    node->mRefCount = last->mRefCount;
                    node->mSize = last->mSize;
                    node->mReleaseOnDestroy = last->mReleaseOnDestroy;
                    node = nullptr;

                    HashNode* newList = LFNew<HashNode>();
                    memcpy(newList, key.mList, sizeof(HashNode) * newSize);
                    LFFree(key.mList);
                    key.mList = newList;
                    key.mPrimary.mSize = static_cast<UInt16>(newSize);
                }
            }
        }
    }
}

void TokenTable::Initialize()
{
    AssertEx(mMap == nullptr, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    mMap = static_cast<HashKey*>(LFAlloc(sizeof(HashKey) * TOKEN_TABLE_SIZE, alignof(HashKey)));
    mMapSize = TOKEN_TABLE_SIZE;
    for (SizeT i = 0; i < mMapSize; ++i)
    {
        mMap[i] = HashKey();
    }
    mLock.Initialize();
}
void TokenTable::Release()
{
    AssertEx(mMap != nullptr, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    mLock.Destroy();
    // Release all string memory...
    // Release all nodes...
    // Release all keys...

    for (SizeT k = 0; k < mMapSize; ++k)
    {
        HashKey& key = mMap[k];
        if (key.UsePrimary())
        {
            if (key.mPrimary.mString && key.mPrimary.mReleaseOnDestroy)
            {
                LFFree(key.mPrimary.mString);
            }
        }
        else
        {
            SizeT size = key.GetSize();
            for (SizeT i = 0; i < size; ++i)
            {
                HashNode& node = key.mList[i];
                if (node.mString && node.mReleaseOnDestroy)
                {
                    LFFree(node.mString);
                }
            }
            // Assumed static.. This memory will be freed with the process!
            LFFree(key.mList);
            key.mList = nullptr;
            key.mPrimary.mSize = 0;
        }
    }
    // Assumed static..
    LFFree(mMap);
    mMap = nullptr;
    mMapSize = 0;
}
void TokenTable::Shutdown()
{
    Release();
    mShutdown = true;
}

bool TokenTable::AcquireNode(Token& token, HashKey& key, const char* begin, const char* end, UInt16 hashIdx)
{
    if (key.UsePrimary())
    {
        char* other = key.mPrimary.mString;
        char* otherEnd = other + key.mPrimary.mSize;
        if (StrEqual(begin, other, end, otherEnd))
        {
            token.mKey = hashIdx;
            token.mString = other;
            token.mSize = static_cast<UInt16>(end - begin);
            _InterlockedIncrement(&key.mPrimary.mRefCount);
            return true;
        }
    }
    else
    {
        const SizeT size = key.GetSize();
        for (SizeT i = 0; i < size; ++i)
        {
            HashNode& node = key.mList[i];
            char* other = node.mString;
            char* otherEnd = other + node.mSize;
            if (StrEqual(begin, other, end, otherEnd))
            {
                token.mKey = hashIdx;
                token.mString = other;
                token.mSize = static_cast<UInt16>(end - begin);
                _InterlockedIncrement(&key.mList[i].mRefCount);
                return true;
            }
        }
    }
    return false;
}
TokenTable::HashNode* TokenTable::AcquireNode(Token& token, HashKey& key)
{
    if (key.UsePrimary())
    {
        return &key.mPrimary;
    }
    else
    {
        const SizeT size = key.GetSize();
        for (SizeT i = 0; i < size; ++i)
        {
            // Token should have localized string, so this is valid compare!
            if (key.mList[i].mString == token.mString)
            {
                return &key.mList[i];
            }
        }
    }
    return nullptr;
}
TokenTable::HashNode& TokenTable::AllocateNode(HashKey& key)
{
    if (key.GetSize() == 0)
    {
        return key.mPrimary;
    }
    
    // Reallocate from using single to chained tokens:
    if (key.UsePrimary())
    {
        HashNode data = key.mPrimary;
        key.mPrimary = HashNode();
        key.mList = static_cast<HashNode*>(LFAlloc(2 * sizeof(HashNode), 16));
        key.mList[0] = data;
        key.mPrimary.mSize = 2;
        return key.mList[1];
    }
    else
    {
        HashNode* oldList = key.mList;
        const SizeT oldSize = key.GetSize();
        HashNode* newList = static_cast<HashNode*>(LFAlloc((oldSize + 1) * sizeof(HashNode), 16));
        if (oldList)
        {
            for (SizeT i = 0; i < oldSize; ++i)
            {
                newList[i] = oldList[i];
            }
            LFFree(oldList);
        }
        else
        {
            newList[0] = key.mPrimary;
            key.mPrimary.mString = nullptr;
            key.mPrimary.mRefCount = 0;
            key.mPrimary.mReleaseOnDestroy = false;
        }
        key.mList = newList;
        key.mPrimary.mSize = static_cast<UInt16>(oldSize + 1);
        return newList[oldSize];
    }
}

void Token::Clear()
{
    if (mString != gNullString)
    {
        DecrementRef();
        mString = gNullString;
        mKey = INVALID16;
        mSize = 0;
    }
}
const char* Token::CStr() const
{
    return mString;
}
SizeT Token::Size() const
{
    return mSize;
}
bool Token::Empty() const
{
    return mString == gNullString;
}

bool Token::AlphaLess(const String& string) const
{
    return StrAlphaLess(mString, string.CStr());
}
bool Token::AlphaLess(const value_type* string) const
{
    return StrAlphaLess(mString, string);
}
bool Token::AlphaGreater(const String& string) const
{
    return StrAlphaGreater(mString, string.CStr());
}
bool Token::AlphaGreater(const value_type* string) const
{
    return StrAlphaGreater(mString, string);
}
bool Token::Compare(const String& string) const
{
    return StrEqual(mString, string.begin(), mString + mSize, string.end());
}
bool Token::Compare(const value_type* string)
{
    return StrEqual(mString, string, mString + mSize, string + StrLen(string));
}

SizeT Token::Find(value_type character) const
{
    return StrFind(mString, mString + mSize, character);
}
SizeT Token::Find(const String& string) const
{
    return StrFind(mString, mString + mSize, string.begin(), string.end());
}
SizeT Token::Find(const value_type* string) const
{
    return StrFind(mString, mString + mSize, string);
}
SizeT Token::FindAgnostic(value_type character) const
{
    return StrFindAgnostic(mString, mString + mSize, character);
}
SizeT Token::FindAgnostic(const String& string) const
{
    return StrFindAgnostic(mString, mString + mSize, string.begin(), string.end());
}
SizeT Token::FindAgnostic(const value_type* string) const
{
    return StrFindAgnostic(mString, mString + mSize, string);
}
SizeT Token::FindLast(value_type character) const
{
    return StrFindLast(mString, mString + mSize, character);
}
SizeT Token::FindLast(const String& string) const
{
    return StrFindLast(mString, mString + mSize, string.begin(), string.end());
}
SizeT Token::FindLast(const value_type* string) const
{
    return StrFindLast(mString, mString + mSize, string);
}
SizeT Token::FindLastAgnostic(value_type character) const
{
    return StrFindLastAgnostic(mString, mString + mSize, character);
}
SizeT Token::FindLastAgnostic(const String& string) const
{
    return StrFindLastAgnostic(mString, mString + mSize, string.begin(), string.end());
}
SizeT Token::FindLastAgnostic(const value_type* string) const
{
    return StrFindLastAgnostic(mString, mString + mSize, string);
}
void Token::DecrementRef()
{
    gTokenTable->DecrementReference(*this);
}
void Token::IncrementRef()
{
    gTokenTable->IncrementReference(*this);
}
void Token::LookUp(const value_type* string, AcquireTag)
{
    gTokenTable->LookUp(string, *this, ACQUIRE);
}
void Token::LookUp(const value_type* string, CopyOnWriteTag)
{
    gTokenTable->LookUp(string, *this, COPY_ON_WRITE);
}
void Token::LookUp(const value_type* string)
{
    gTokenTable->LookUp(string, *this);
}
void Token::LookUp(const String& string)
{
    gTokenTable->LookUp(string.CStr(), *this);
}
void Token::LookUp(const String& string, AcquireTag)
{
    gTokenTable->LookUp(string.CStr(), *this, ACQUIRE);
}

} // namespace lf
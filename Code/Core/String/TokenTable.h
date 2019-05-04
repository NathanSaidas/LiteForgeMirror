#ifndef LF_CORE_TOKEN_TABLE_H
#define LF_CORE_TOKEN_TABLE_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/Platform/CriticalSection.h"

namespace lf {

class Token;
class TokenTable;
LF_CORE_API extern TokenTable* gTokenTable;

class LF_CORE_API TokenTable
{
public:
    struct HashNode
    {
        HashNode() :
            mString(nullptr),
            mRefCount(0),
            mReleaseOnDestroy(false),
            mSize(0)
        {}
        char* mString;
        long  mRefCount;
        UInt16 mSize;
        bool   mReleaseOnDestroy;
    };
    struct HashKey
    {
        HashKey() :
            mPrimary(),
            mList(nullptr)
        {}
        bool UsePrimary() const { return mList == nullptr; }
        SizeT GetSize() const { return UsePrimary() ? (mPrimary.mString ? 1 : 0) : mPrimary.mSize; }

        HashNode mPrimary; // Note: mPrimary.mSize == sizeof list when mList != nullptr
        HashNode* mList;
    };
    TokenTable();
    ~TokenTable();

    // Look up token info. ( Do not allocate if it doesn't exist. )
    void LookUp(const char* string, Token& token, AcquireTag);
    // Look up token info. ( Will not copy the string to StringHeap )
    void LookUp(const char* string, Token& token, CopyOnWriteTag);
    // Look up token info. ( Will copy to StringHeap if it does not exist. )
    void LookUp(const char* string, Token& token);

    void IncrementReference(Token& token);
    void DecrementReference(Token& token);

    void Initialize();
    void Release();
    void Shutdown();
private:
    bool AcquireNode(Token& token, HashKey& key, const char* begin, const char* end, UInt16 hashIdx);
    HashNode* AcquireNode(Token& token, HashKey& key);
    HashNode& AllocateNode(HashKey& key);

    HashKey* mMap;
    SizeT    mMapSize;

    CriticalSection mLock;
    bool mShutdown;
};


} // namespace lf

#endif // LF_CORE_TOKEN_TABLE_H
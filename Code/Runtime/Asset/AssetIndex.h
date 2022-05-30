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
#ifndef LF_CORE_ASSET_INDEX_H
#define LF_CORE_ASSET_INDEX_H

#include "Core/Utility/StdVector.h"
#include <algorithm>

namespace lf {

template<typename KeyT, typename IndexT>
struct TAssetIndexTraits
{
    using Compare = std::less<KeyT>;
    using KeyType = KeyT;
    using IndexType = IndexT;

    static KeyT DefaultKey() { return KeyT(); }
    static IndexT DefaultIndex() { return IndexT(); }
};

// Object Oriented index
template<typename KeyT, typename IndexT>
using TAssetPairIndex = TVector<std::pair<KeyT, IndexT>>;

// Data Oriented Index
template<typename KeyT, typename IndexT, typename TraitsT = TAssetIndexTraits<KeyT, IndexT>>
class TAssetIndex
{
public:
    using KeyType = KeyT;
    using IndexType = IndexT;
    using TraitsType = TraitsT;
    using CompareType = typename TraitsT::Compare;
    using BuilderDataType = TAssetPairIndex<KeyType, IndexType>;
    using PairType = std::pair<KeyT, IndexT>;

    // **********************************
    // Builds the index assuming 'data' is sorted and each key is unique
    // **********************************
    void Build(const TAssetPairIndex<KeyType, IndexType>& data)
    {
        Clear();

        const SizeT tableSize = data.size();
        mKeys.reserve(tableSize);
        mKeys.resize(tableSize);
        mIndices.reserve(tableSize);
        mIndices.resize(tableSize);

        for (SizeT i = 0; i < tableSize; ++i)
        {
            mKeys[i] = data[i].first;
            mIndices[i] = data[i].second;
        }
    }

    // **********************************
    // Find an index associated with a key, 
    // @returns Returns the 'DefaultIndex' on if the key does not exist.
    // **********************************
    IndexType Find(const KeyType& key) const
    {
        CompareType comp = {};
        auto it = std::lower_bound(mKeys.begin(), mKeys.end(), key);
        auto result = it != mKeys.end() && !comp(key, *it) ? it : mKeys.end();
        if (result != mKeys.end())
        {
            auto index = it - mKeys.begin();
            return mIndices[index];
        }
        return TraitsT::DefaultIndex();
    }

    IndexType& FindRef(const KeyType& key)
    {
        CompareType comp = {};
        auto it = std::lower_bound(mKeys.begin(), mKeys.end(), key);
        auto result = it != mKeys.end() && !comp(key, *it) ? it : mKeys.end();
        Assert(result != mKeys.end());
        auto index = it - mKeys.begin();
        return mIndices[index];
    }

    // **********************************
    // Clears all keys/indices from the index
    // **********************************
    void Clear()
    {
        mKeys.clear();
        mIndices.clear();
    }

    // **********************************
    // @returns The number of data mapped 
    // **********************************
    SizeT Size() const { return mKeys.Size(); }

    // **********************************
    // Calculates the number of bytes used by the index, using to functions to
    // extract full size data out of the key/index
    //
    // @param keySize -- TCallback<SizeT,const KeyType&>
    // @param indexSize -- TCallback<SizeT, const IndexType&>
    // **********************************
    SizeT QueryFootprint(SizeT(*keySize)(const KeyType&), SizeT(*indexSize)(const IndexType&)) const
    {
        SizeT footprint = 0;
        footprint += mKeys.Size() * sizeof(KeyType);
        footprint += mIndices.Size() * sizeof(IndexType);
        for (SizeT i = 0, size = Size(); i < size; ++i)
        {
            footprint += keySize(mKeys[i]);
            footprint += indexSize(mIndices[i]);
        }
        return footprint;
    }
private:
    TVector<KeyType>     mKeys;
    TVector<IndexType>   mIndices;
};

namespace AssetUtil
{
    struct DefaultNameIndexTraits
    {
        using Base = TAssetIndexTraits<const char*, UInt32>;
        using Compare = typename Base::Compare;
        using KeyType = typename Base::KeyType;
        using IndexType = typename Base::IndexType;
        using BuilderType = TAssetPairIndex<KeyType, IndexType>;

        static KeyType DefaultKey() { return ""; }
        static IndexType DefaultIndex() { return INVALID32; }
    };
    using DefaultNameIndex = TAssetIndex<DefaultNameIndexTraits::KeyType, DefaultNameIndexTraits::IndexType, DefaultNameIndexTraits>;
    struct DefaultUIDIndexTraits
    {
        using Base = TAssetIndexTraits<UInt32, UInt32>;
        using Compare = typename Base::Compare;
        using KeyType = typename Base::KeyType;
        using IndexType = typename Base::IndexType;
        using BuilderType = TAssetPairIndex<KeyType, IndexType>;

        static KeyType DefaultKey() { return INVALID32; }
        static IndexType DefaultIndex() { return INVALID32; }
    };
    using DefaultUIDIndex = TAssetIndex<DefaultUIDIndexTraits::KeyType, DefaultUIDIndexTraits::IndexType, DefaultUIDIndexTraits>;

}

}

#endif // LF_CORE_ASSET_INDEX_H
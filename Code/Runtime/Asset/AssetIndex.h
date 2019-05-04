#ifndef LF_CORE_ASSET_INDEX_H
#define LF_CORE_ASSET_INDEX_H

#include "Core/Utility/Array.h"
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
using TAssetPairIndex = TArray<std::pair<KeyT, IndexT>>;

// Data Oriented Index
template<typename KeyT, typename IndexT, typename TraitsT = AssetIndexTraits<KeyT, IndexT>>
class TAssetIndex
{
public:
    using KeyType = KeyT;
    using IndexType = IndexT;
    using TraitsType = TraitsT;
    using CompareType = typename TraitsT::Compare;

    // **********************************
    // Builds the index assuming 'data' is sorted and each key is unique
    // **********************************
    void Build(const TAssetPairIndex<KeyType, IndexType>& data)
    {
        Clear();

        const SizeT tableSize = data.Size();
        mKeys.Reserve(tableSize);
        mKeys.Resize(tableSize);
        mIndices.Reserve(tableSize);
        mIndices.Resize(tableSize);

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

    // **********************************
    // Clears all keys/indices from the index
    // **********************************
    void Clear()
    {
        mKeys.Clear();
        mIndices.Clear();
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
    TArray<KeyType>     mKeys;
    TArray<IndexType>   mIndices;
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
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
#ifndef LF_CORE_ARRAY_LIST_H
#define LF_CORE_ARRAY_LIST_H

#include "Core/Common/Assert.h"
#include "Core/Memory/Memory.h"
#include "Core/Utility/ErrorCore.h"

namespace lf {

    namespace internal_util {
        LF_INLINE const SizeT ElementToBlockCount(const SizeT elementCount, const SizeT blockSize)
        {
            return (elementCount < blockSize) ? 1 : ((elementCount + (elementCount % blockSize)) / blockSize);
        }
        const SizeT ARRAY_LIST_MAX_BLOCK_SIZE = 64;
        const SizeT ARRAY_LIST_REND_ID = 0xBBBB;
        const SizeT ARRAY_LIST_END_ID = 0xEEEE;
    }

    // A combination of list and array. This container uses small chunks of continugous blocks of memory in a linked list fashion
    // which has the benefit of items inside the container having a valid pointer even when the container is modified! However because
    // not all memory is guaranteed to be contiguous memory access can be slightly slower, also insertion semantics are not possible
    // and random access is not fast.

    template<typename T>
    struct TArrayListBlockState
    {
        using mask_type = UInt64;

        TArrayListBlockState() :
            mPrevious(nullptr),
            mNext(nullptr),
            mItemMask(0)
        {

        }

        TArrayListBlockState* mPrevious;
        TArrayListBlockState* mNext;
        mask_type mItemMask; // Determines if element is used
    };

    template<typename T, SizeT BLOCK_SIZE_T>
    struct TArrayListBlock
    {
        using value_type = T;
        using mask_type = typename TArrayListBlockState<T>::mask_type;
        using block_state = TArrayListBlockState<T>;
        static const SizeT BLOCK_SIZE = BLOCK_SIZE_T;

        TArrayListBlock() :
            mState(),
            mItems()
        {
            LF_STATIC_ASSERT(BLOCK_SIZE > 0 && BLOCK_SIZE <= internal_util::ARRAY_LIST_MAX_BLOCK_SIZE);
        }
        block_state mState;
        value_type  mItems[BLOCK_SIZE];

    };

    template<typename T>
    class TArrayListConstIterator
    {
    public:
        using block_state = TArrayListBlockState<T>;
        using mask_type = typename TArrayListBlockState<T>::mask_type;
        using container_type = void;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator_category = std::random_access_iterator_tag;

        TArrayListConstIterator() :
            mState(nullptr),
            mItemIndex(0),
            mStateOffset(0),
            mBlockSize(0),
            mContainer(nullptr)
        {

        }

        TArrayListConstIterator(const TArrayListConstIterator& other) :
            mState(other.mState),
            mItemIndex(other.mItemIndex),
            mStateOffset(other.mStateOffset),
            mBlockSize(other.mBlockSize),
            mContainer(other.mContainer)
        {

        }

        explicit TArrayListConstIterator(block_state* state, SizeT index, UInt16 offset, UInt16 blockSize, const container_type* container) :
            mState(state),
            mItemIndex(index),
            mStateOffset(offset),
            mBlockSize(blockSize),
            mContainer(container)
        {
            AdvanceValid();
        }

        ~TArrayListConstIterator()
        {

        }

        TArrayListConstIterator& operator=(const TArrayListConstIterator& other)
        {
            mState = other.mState;
            mStateOffset = other.mStateOffset;
            mBlockSize = other.mBlockSize;
            mItemIndex = other.mItemIndex;
            mContainer = other.mContainer;
            return *this;
        }

        void swap(TArrayListConstIterator& other)
        {
            std::swap(mState, other.mState);
            std::swap(mStateOffset, other.mStateOffset);
            std::swap(mBlockSize, other.mBlockSize);
            std::swap(mItemIndex, other.mItemIndex);
            std::swap(mContainer, other.mContainer);
        }

        bool operator!=(const TArrayListConstIterator& other) const
        {
            return mState ? (mState != other.mState || mItemIndex != other.mItemIndex) : (mItemIndex != other.mItemIndex);
        }
        bool operator==(const TArrayListConstIterator& other) const
        {
            return mState ? (mState == other.mState && mItemIndex == other.mItemIndex) : (mItemIndex == other.mItemIndex);
        }
        const_reference operator*() const
        {
#if defined(LF_ITERATOR_RANGE_CHECK)
            Assert(mState && mItemIndex != internal_util::ARRAY_LIST_END_ID && mItemIndex != internal_util::ARRAY_LIST_REND_ID);
#endif
            return GetArray()[mItemIndex];
        }
        const_pointer operator->() const
        {
#if defined(LF_ITERATOR_RANGE_CHECK)
            Assert(mState && mItemIndex != internal_util::ARRAY_LIST_END_ID && mItemIndex != internal_util::ARRAY_LIST_REND_ID);
#endif
            return &(GetArray()[mItemIndex]);
        }
        const_reference operator[](difference_type n) const
        {
            // todo: Verify this is correct!
            TArrayListConstIterator tmp(*this);
            if (n > 0)
            {
                tmp += n;
                return *tmp;
            }
            else
            {
                tmp -= math::Abs(n);
                return *tmp;
            }
        }

        // pre-increment
        TArrayListConstIterator& operator++()
        {
            Increment();
            return *this;
        }
        // pre-decrement
        TArrayListConstIterator& operator--()
        {
            Decrement();
            return *this;
        }

        TArrayListConstIterator operator++(int)
        {
            TArrayListConstIterator tmp(*this);
            ++tmp;
            return tmp;
        }

        TArrayListConstIterator operator--(int)
        {
            TArrayListConstIterator tmp(*this);
            --tmp;
            return tmp;
        }

        TArrayListConstIterator operator+=(SizeT n)
        {
            for (SizeT i = 0; i < n; ++i)
            {
                Increment();
            }
            return *this;
        }

        TArrayListConstIterator operator+(SizeT n)
        {
            TArrayListConstIterator tmp(*this);
            tmp += n;
            return tmp;
        }

        TArrayListConstIterator operator-=(SizeT n)
        {
            for (SizeT i = 0; i < n; ++i)
            {
                Decrement();
            }
            return *this;
        }

        TArrayListConstIterator operator-(SizeT n)
        {
            TArrayListConstIterator tmp(*this);
            tmp -= n;
            return tmp;
        }

        difference_type operator-(const TArrayListConstIterator& other) const
        {
            difference_type blockIndex = static_cast<difference_type>(GetElementIndex());
            difference_type otherBlockIndex = static_cast<difference_type>(other.GetElementIndex());
            return blockIndex - otherBlockIndex;
        }

        bool operator<(const TArrayListConstIterator& other) const
        {
            // todo: Whats the logic for 'begin < end'
            SizeT blockIndex = GetBlockIndex();
            SizeT otherBlockIndex = other.GetBlockIndex();
            if (blockIndex < otherBlockIndex)
            {
                return true;
            }
            return mItemIndex < other.mItemIndex;
        }

        bool operator>(const TArrayListConstIterator& other) const
        {
            // todo: Whats the logic for 'begin > end'
            SizeT blockIndex = GetBlockIndex();
            SizeT otherBlockIndex = other.GetBlockIndex();
            if (blockIndex > otherBlockIndex)
            {
                return true;
            }
            return mItemIndex > other.mItemIndex;
        }

        bool operator<=(const TArrayListConstIterator& other) const
        {
            if (*this == other)
            {
                return true;
            }
            // todo: Whats the logic for 'begin < end'
            SizeT blockIndex = GetBlockIndex();
            SizeT otherBlockIndex = other.GetBlockIndex();
            if (blockIndex < otherBlockIndex)
            {
                return true;
            }
            return mItemIndex < other.mItemIndex;
        }

        bool operator>=(const TArrayListConstIterator& other) const
        {
            if (*this == other)
            {
                return true;
            }
            // todo: Whats the logic for `begin > end`
            SizeT blockIndex = GetBlockIndex();
            SizeT otherBlockIndex = other.GetBlockIndex();
            if (blockIndex > otherBlockIndex)
            {
                return true;
            }
            return mItemIndex > other.mItemIndex;
        }


        SizeT GetBlockIndex() const
        {
            SizeT index = 0;
            block_state* tmp = mState;
            while (tmp != nullptr)
            {
                tmp = tmp->mPrevious;
                ++index;
            }
            return index;
        }

        SizeT GetElementIndex() const
        {
            if (Invalid(mItemIndex))
            {
                return INVALID;
            }

            // figure out where we are locally:
            SizeT localIndex = 0;
            if (mItemIndex != 0)
            {
                mask_type mask = mask_type(1) << (mItemIndex - 1);
                while (mask != 0)
                {
                    if ((mask & mState->mItemMask) != 0)
                    {
                        ++localIndex;
                    }
                    mask = mask >> mask_type(1);
                }
            }

            // figure out where we are "globally"
            SizeT index = 0;
            block_state* tmp = mState->mPrevious;
            while (tmp != nullptr)
            {
                index += BitCount(tmp->mItemMask);
                tmp = tmp->mPrevious;
            }
            return localIndex + index;
        }

#if defined(LF_ITERATOR_CONTAINER_CHECK)
        const container_type* GetContainer() const
        {
            return mContainer;
        }
#endif
        const block_state* GetState() const
        {
            return mState;
        }
        SizeT GetItemIndex() const
        {
            return mItemIndex;
        }

        pointer GetItem()
        {
            return &(GetArray()[mItemIndex]);
        }

    protected:
        void AdvanceValid()
        {
            if (!mState || mItemIndex == internal_util::ARRAY_LIST_END_ID)
            {
                return;
            }
            mask_type mask = mask_type(1) << mItemIndex;
            if ((mask & mState->mItemMask) > 0)
            {
                return;
            }
            Increment();
        }
        void Increment()
        {
#if defined(LF_ITERATOR_RANGE_CHECK)
            Assert(mState && mItemIndex != internal_util::ARRAY_LIST_END_ID);
#endif
            // Recover from begin - 1
            if (mItemIndex == internal_util::ARRAY_LIST_REND_ID)
            {
                mItemIndex = 0;
            }
            else
            {
                ++mItemIndex;
            }

            mask_type mask = mask_type(1) << (mItemIndex);
            // while has state && not valid element
            while ((mask & mState->mItemMask) == 0)
            {
                ++mItemIndex; // increment

                              // if at end of block, move next
                if (mItemIndex >= mBlockSize)
                {
                    if (mState->mNext == nullptr)
                    {
                        mItemIndex = internal_util::ARRAY_LIST_END_ID;
                        return; // we are now end!
                    }
                    else
                    {
                        mState = mState->mNext;
                    }
                    mItemIndex = 0;
                }
                mask = mask_type(1) << mItemIndex; // refresh mask
            }
        }

        void Decrement()
        {
#if defined(LF_ITERATOR_RANGE_CHECK)
            Assert(mState && mItemIndex != internal_util::ARRAY_LIST_REND_ID);
#endif
            if (mItemIndex == internal_util::ARRAY_LIST_END_ID)
            {
                mItemIndex = mBlockSize;
            }
            --mItemIndex;
            mask_type mask = mask_type(1) << mItemIndex;

            while ((mask & mState->mItemMask) == 0)
            {
                if (Invalid(mItemIndex))
                {
                    if (mState->mPrevious == nullptr)
                    {
                        mItemIndex = internal_util::ARRAY_LIST_REND_ID;
                        return; // we are now one before begin
                    }
                    else
                    {
                        mState = mState->mPrevious;
                    }
                    mItemIndex = mBlockSize - 1;
                }
                else
                {
                    --mItemIndex;
                }
                mask = mask_type(1) << mItemIndex; // refresh mask
            }
        }

        mask_type GetIndexMask() const
        {
            return 1 << mItemIndex;
        }

        const_pointer GetArray() const
        {
            return reinterpret_cast<const_pointer>(AddressAdd(mState, mStateOffset));
        }
        pointer GetArray()
        {
            return reinterpret_cast<pointer>(AddressAdd(mState, mStateOffset));
        }

        block_state* mState;
        SizeT        mItemIndex;
        UInt16       mStateOffset; // static_on_init
        UInt16       mBlockSize;   // static_on_init
#if defined(LF_ITERATOR_RANGE_CHECK) || defined(LF_ITERATOR_CONTAINER_CHECK)
        const container_type* mContainer; // static_on_init
#endif
    };

    template<typename T>
    class TArrayListIterator : public TArrayListConstIterator<T>
    {
    public:
        using block_state = TArrayListBlockState<T>;
        using mask_type = typename TArrayListBlockState<T>::mask_type;
        using container_type = void;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator_category = std::random_access_iterator_tag;
        using super = TArrayListConstIterator<T>;

        TArrayListIterator() : super()
        {}
        TArrayListIterator(const TArrayListIterator& other) : super(other)
        {}
        explicit TArrayListIterator(block_state* state, SizeT index, UInt16 offset, UInt16 blockSize, const container_type* container) :
            super(state, index, offset, blockSize, container)
        {}
        ~TArrayListIterator()
        {}

        reference operator*()
        {
            return GetArray()[mItemIndex];
        }
        pointer operator->()
        {
            return &(GetArray()[mItemIndex]);
        }
        reference operator[](difference_type n)
        {
            // todo: Verify this is correct!
            TArrayListIterator tmp(*this);
            if (n > 0)
            {
                tmp += n;
                return *tmp;
            }
            else
            {
                tmp -= math::Abs(n);
                return *tmp;
            }
        }

        TArrayListIterator& operator++()
        {
            Increment();
            return *this;
        }
        TArrayListIterator& operator--()
        {
            Decrement();
            return *this;
        }

        TArrayListIterator operator++(int)
        {
            TArrayListIterator tmp(*this);
            ++tmp;
            return tmp;
        }

        TArrayListIterator operator--(int)
        {
            TArrayListIterator tmp(*this);
            --tmp;
            return tmp;
        }

        TArrayListIterator& operator+=(SizeT n)
        {
            for (SizeT i = 0; i < n; ++i)
            {
                Increment();
            }
            return *this;
        }
        TArrayListIterator& operator-=(SizeT n)
        {
            for (SizeT i = 0; i < n; ++i)
            {
                Decrement();
            }
            return *this;
        }
        TArrayListIterator operator+(SizeT n)
        {
            TArrayListIterator tmp(*this);
            tmp += n;
            return tmp;
        }
        TArrayListIterator operator-(SizeT n)
        {
            TArrayListIterator tmp(*this);
            tmp -= n;
            return tmp;
        }

        difference_type operator-(const TArrayListIterator& other) const
        {
            difference_type blockIndex = static_cast<difference_type>(GetElementIndex());
            difference_type otherBlockIndex = static_cast<difference_type>(other.GetElementIndex());
            return blockIndex - otherBlockIndex;
        }
    };


    template<typename T, SizeT BLOCK_SIZE>
    class TArrayList
    {
    public:
        using value_type = T;
        using difference_type = ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator = TArrayListIterator<T>;
        using const_iterator = TArrayListConstIterator<T>;
        using block_type = TArrayListBlock<T, BLOCK_SIZE>;

        TArrayList() :
            mFirst(nullptr),
            mLast(nullptr),
            mItemCount(0),
            mBlockCount(0)
        {
        }

        TArrayList(const TArrayList& other) :
            mFirst(nullptr),
            mLast(nullptr),
            mItemCount(0),
            mBlockCount(0)
        {
            AddRange(other.begin(), other.end());
        }

        TArrayList(TArrayList&& other) :
            mFirst(other.mFirst),
            mLast(other.mLast),
            mItemCount(other.mItemCount),
            mBlockCount(other.mBlockCount)
        {
            other.mFirst = other.mLast = nullptr;
            other.mItemCount = other.mBlockCount = 0;
        }

        TArrayList(const std::initializer_list<T>& items) :
            mFirst(nullptr),
            mLast(nullptr),
            mItemCount(0),
            mBlockCount(0)
        {
            AddRange(items.begin(), items.end());
        }

        template<typename IteratorT>
        TArrayList(IteratorT first, IteratorT last) :
            mFirst(nullptr),
            mLast(nullptr),
            mItemCount(0),
            mBlockCount(0)
        {
            AddRange(first, last);
        }

        ~TArrayList()
        {
            Clear();
        }

        void swap(TArrayList& other)
        {
            std::swap(mFirst, other.mFirst);
            std::swap(mLast, other.mLast);
            std::swap(mItemCount, other.mItemCount);
            std::swap(mBlockCount, other.mBlockCount);
        }

        iterator begin()
        {
            return mFirst ?
                iterator(&mFirst->mState, 0, GetItemsOffset(), BLOCK_SIZE, this) :
                iterator(nullptr, internal_util::ARRAY_LIST_END_ID, GetItemsOffset(), BLOCK_SIZE, this);
        }

        iterator end()
        {
            return mLast ?
                iterator(&mLast->mState, internal_util::ARRAY_LIST_END_ID, GetItemsOffset(), BLOCK_SIZE, this) :
                iterator(nullptr, internal_util::ARRAY_LIST_END_ID, GetItemsOffset(), BLOCK_SIZE, this);
        }

        const_iterator begin() const
        {
            return mFirst ?
                const_iterator(&mFirst->mState, 0, GetItemsOffset(), BLOCK_SIZE, this) :
                const_iterator(nullptr, internal_util::ARRAY_LIST_END_ID, GetItemsOffset(), BLOCK_SIZE, this);
        }

        const_iterator end() const
        {
            return mLast ?
                const_iterator(&mLast->mState, internal_util::ARRAY_LIST_END_ID, GetItemsOffset(), BLOCK_SIZE, this) :
                const_iterator(nullptr, internal_util::ARRAY_LIST_END_ID, GetItemsOffset(), BLOCK_SIZE, this);
        }

        const_iterator cbegin() const
        {
            return mFirst ?
                const_iterator(&mFirst->mState, 0, GetItemsOffset(), BLOCK_SIZE, this) :
                const_iterator(nullptr, internal_util::ARRAY_LIST_END_ID, GetItemsOffset(), BLOCK_SIZE, this);
        }

        const_iterator cend() const
        {
            return mLast ?
                const_iterator(&mLast->mState, internal_util::ARRAY_LIST_END_ID, GetItemsOffset(), BLOCK_SIZE, this) :
                const_iterator(nullptr, internal_util::ARRAY_LIST_END_ID, GetItemsOffset(), BLOCK_SIZE, this);
        }

        bool Empty() const { return mItemCount == 0; }
        SizeT Size() const { return mItemCount; }
        SizeT Capacity() const { return mBlockCount * BLOCK_SIZE; }
        static constexpr SizeT DefaultCapacity() { return 0; }
        SizeT GetBlockCount() const { return mBlockCount; }

        // I'm not sure this makes sense...
        // void Resize(SizeT size);

        // We can grow to X blocks! We can never shrink though!
        void Reserve(const SizeT elementCount);
        void Clear();

        iterator Add(const value_type& item);
        iterator Remove(iterator it);

        template<typename IterT>
        void AddRange(IterT first, IterT last)
        {
            while (first != last)
            {
                Add(*first);
                ++first;
            }
        }

        reference GetLast()
        {
            Assert(!Empty());
            iterator last = end() - 1;
            return *last;
        }
        const_reference GetLast() const
        {
            Assert(!Empty());
            iterator last = end() - 1;
            return *last;
        }

        reference GetFirst()
        {
            Assert(!Empty());
            iterator first = begin();
            return *first;
        }

        const_reference GetFirst() const
        {
            Assert(!Empty());
            iterator first = begin();
            return *first;
        }

        TArrayList& operator=(const TArrayList& other)
        {
            Clear();
            AddRange(other.begin(), other.end());
            return *this;
        }

        TArrayList& operator=(TArrayList&& other)
        {
            Clear();
            mFirst = other.mFirst;
            mLast = other.mLast;
            mItemCount = other.mItemCount;
            mBlockCount = other.mBlockCount;

            other.mFirst = nullptr;
            other.mLast = nullptr;
            other.mItemCount = 0;
            other.mBlockCount = 0;
            return *this;
        }

        bool operator==(const TArrayList& other) const
        {
            if (Size() != other.Size())
            {
                return false;
            }
            const_iterator x = begin();
            const_iterator y = other.begin();
            const_iterator xEnd = end();
            while (x != xEnd)
            {
                if (*x != *y)
                {
                    return false;
                }
                ++x;
                ++y;
            }
            return true;
        }

        bool operator!=(const TArrayList& other) const
        {
            return !(*this == other);
        }

        reference operator[](difference_type i)
        {
            return begin()[i];
        }

        const_reference operator[](difference_type i) const
        {
            return begin()[i];
        }
    private:
        LF_FORCE_INLINE const UInt16 GetItemsOffset() const { return offsetof(block_type, mItems); }
        LF_FORCE_INLINE void Destroy(pointer ptr)
        {
            ptr->~value_type();
            (ptr);
        }
        LF_FORCE_INLINE void Construct(pointer ptr)
        {
            new(ptr)value_type();
        }
        block_type* GetFirstFreeBlock()
        {
            block_type* block = mFirst;
            while (block)
            {
                SizeT used = BitCount(block->mState.mItemMask);
                if (used != BLOCK_SIZE)
                {
                    break;
                }
                block = reinterpret_cast<block_type*>(block->mState.mNext);
            }
            return block;
        }
        block_type* AllocateBlock()
        {
            // allocate:
            block_type* block = LFNew<block_type>();

            // link:
            if (mFirst == nullptr)
            {
                mFirst = block;
                mLast = block;
            }
            else
            {
                mLast->mState.mNext = &block->mState;
                block->mState.mPrevious = &mLast->mState;
                mLast = block;
            }
            ++mBlockCount;
            return block;
        }
        void DestroyBlock(block_type* block)
        {
            block_type* prev = reinterpret_cast<block_type*>(block->mState.mPrevious);
            block_type* next = reinterpret_cast<block_type*>(block->mState.mNext);

            // unlink:
            // cases:
            // block == first
            // block == middle
            // block == last

            // first: prev == null && next == maybe null
            if (block == mFirst)
            {
                if (next)
                {
                    mFirst = next;
                    next->mState.mPrevious = nullptr;
                }
                else
                {
                    mFirst = mLast = nullptr;
                    Assert(mItemCount == 0);
                    Assert(mBlockCount == 1);
                }
            }
            else if (block == mLast)
            {
                Assert(prev);
                Assert(next == nullptr);
                mLast = prev;
                prev->mState.mNext = nullptr;
            }
            else
            {
                Assert(prev && next);
                prev->mState.mNext = &next->mState;
                next->mState.mPrevious = &prev->mState;
            }

            // deallocate:
            LFDelete(block);
            Assert(mBlockCount > 0);
            --mBlockCount;
        }
#if defined(LF_ITERATOR_RANGE_CHECK)
        bool CheckIterator(const typename block_type::block_state* state)
        {
            if (!mFirst || !state)
            {
                return false;
            }
            typename block_type::block_state* it = &mFirst->mState;
            while (it)
            {
                if (state == it)
                {
                    return true;
                }
                it = it->mNext;
            }
            return false;
        }
#endif

        block_type* mFirst;
        block_type* mLast;
        SizeT       mItemCount;
        SizeT       mBlockCount;

    };

    template<typename T, SizeT BLOCK_SIZE>
    void TArrayList<T, BLOCK_SIZE>::Reserve(const SizeT elementCount)
    {
        SizeT blockCount = internal_util::ElementToBlockCount(elementCount, BLOCK_SIZE);
        if (blockCount <= mBlockCount)
        {
            return;
        }
        while (blockCount > mBlockCount)
        {
            AllocateBlock();
        }
    }

    template<typename T, SizeT BLOCK_SIZE>
    void TArrayList<T, BLOCK_SIZE>::Clear()
    {
        block_type* it = mFirst;
        while (it)
        {
            block_type* target = it;
            it = reinterpret_cast<block_type*>(target->mState.mNext);

            LFDelete(target);
        }

        mFirst = nullptr;
        mLast = nullptr;
        mItemCount = 0;
        mBlockCount = 0;
    }

    template<typename T, SizeT BLOCK_SIZE>
    typename TArrayList<T, BLOCK_SIZE>::iterator TArrayList<T, BLOCK_SIZE>::Add(const value_type& item)
    {
        using mask_type = typename block_type::mask_type;

        // find_free_block
        // if fail
        //    allocate_block
        // find_free_index
        // insert_into_block
        // return iterator
        block_type* block = GetFirstFreeBlock();
        SizeT freeIndex = 0;
        if (block == nullptr)
        {
            block = AllocateBlock();
        }
        else
        {
            mask_type mask = mask_type(1) << freeIndex;
            while ((mask & block->mState.mItemMask) > 0)
            {
                ++freeIndex;
                mask = mask_type(1) << freeIndex;
            }
        }
        block->mItems[freeIndex] = item;
        block->mState.mItemMask |= (mask_type(1) << freeIndex);
        ++mItemCount;
        return iterator(&block->mState, freeIndex, GetItemsOffset(), BLOCK_SIZE, this);
    }

    template<typename T, SizeT BLOCK_SIZE>
    typename TArrayList<T, BLOCK_SIZE>::iterator TArrayList<T, BLOCK_SIZE>::Remove(iterator it)
    {
        using mask_type = typename block_type::mask_type;
#if defined(LF_ITERATOR_CONTAINER_CHECK)
        if (it.GetContainer() != reinterpret_cast<void*>(this))
        {
            CriticalAssertMsgEx("Iterator container mismatch!", LF_ERROR_BAD_STATE, ERROR_API_CORE);
        }
#endif
#if defined(LF_ITERATOR_RANGE_CHECK)
        if (!CheckIterator(it.GetState()) || // Corrupt data:
            (it.GetItemIndex() == internal_util::ARRAY_LIST_END_ID || it.GetItemIndex() == internal_util::ARRAY_LIST_REND_ID)) // Out of range
        {
            CriticalAssertMsgEx("Iterator out of range!", LF_ERROR_BAD_STATE, ERROR_API_CORE);
        }
#endif
        iterator next = it + 1;
        // get block
        block_type* block = const_cast<block_type*>(reinterpret_cast<const block_type*>(it.GetState()));
        SizeT itemIndex = it.GetItemIndex();
        mask_type itemBit = mask_type(1) << itemIndex;
        Assert((block->mState.mItemMask & itemBit) > 0); // Must be set!

                                                         // destroy/init default:
        Destroy(&block->mItems[itemIndex]);
        Construct(&block->mItems[itemIndex]);

        // unset mask and reduce count
        block->mState.mItemMask = block->mState.mItemMask & ~itemBit;
        --mItemCount;

        // check block deallocate
        if (block->mState.mItemMask == 0)
        {
            bool returnEnd = next.GetState() == &block->mState;
            Assert(BitCount(block->mState.mItemMask) == 0);
            DestroyBlock(block);
            if (returnEnd)
            {
                next = end();
            }
        }
        return next;
    }

} // namespace lf

#endif // LF_CORE_ARRAY_LIST_H
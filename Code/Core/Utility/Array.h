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
#ifndef LF_CORE_ARRAY_H
#define LF_CORE_ARRAY_H

// #include "core/system/Assert.h"
// #include "core/system/GenericHeap.h"
// #include "core/system/Memory.h"
// #include "core/util/UtilDef.h"

#include "Core/Common/Types.h"
#include "Core/Common/Assert.h"
#include "Core/Memory/Memory.h"
#include "Core/Utility/ErrorCore.h"
#include <iterator>


#define LF_DEBUG_ITERATOR
// Check if iterators are within a valid range when accessing
#define LF_ITERATOR_RANGE_CHECK
// Check that iterators belong to the same container when compared.
#define LF_ITERATOR_CONTAINER_CHECK
#define LF_ITERATOR_STL_CHECK


namespace lf
{
    

    template<typename T, typename AllocatorT = DefaultAllocator>
    struct TArrayData
    {
        using value_type = T;
        using pointer = T*;
        TArrayData() :
            mFirst(nullptr),
            mLast(nullptr),
            mEnd(nullptr)
        {}
        TArrayData(TArrayData&& other) :
            mFirst(other.mFirst),
            mLast(other.mLast),
            mEnd(other.mEnd)
        {
            other.mFirst = other.mLast = other.mEnd = nullptr;
        }

        TArrayData& operator=(TArrayData&& other)
        {
            mFirst = other.mFirst;
            mLast = other.mLast;
            mEnd = other.mEnd;
            other.mFirst = nullptr;
            other.mLast = nullptr;
            other.mEnd = nullptr;
            return *this;
        }

        void swap(TArrayData& other)
        {
            std::swap(mFirst, other.mFirst);
            std::swap(mLast, other.mLast);
            std::swap(mEnd, other.mEnd);
        }

        void Grow(SizeT size, SizeT reserve = 2)
        {
            const SizeT capacity = mEnd - mFirst;
            if (size <= capacity)
            {
                return;
            }

            size *= reserve; // grow for extra
            const SizeT oldSize = mLast - mFirst;
            // Cases:
            // 1. None > Heap
            // 2. Heap > Heap

            // None = No Copy
            if (capacity == 0)
            {
                mFirst = static_cast<pointer>(AllocatorT::Allocate(sizeof(value_type) * size, alignof(value_type)));
                mLast = mFirst;
                mEnd = mFirst + size;
                return;
            }

            // heap to heap:
            pointer first = static_cast<pointer>(AllocatorT::Allocate(sizeof(value_type) * size, alignof(value_type)));
            pointer last = first + oldSize;
            pointer end = first + size;
            pointer it = first;
            pointer oldIt = mFirst;
            while (it != last)
            {
                it = new(it)value_type();
                *it = *oldIt;
                oldIt->~value_type();
                ++it;
                ++oldIt;
            }
            AllocatorT::Free(mFirst);
            it = first;
            mFirst = first;
            mLast = last;
            mEnd = end;
        }

        void Shrink(SizeT size)
        {
            SizeT capacity = mEnd - mFirst;
            Assert(size <= capacity);
            if (capacity <= size)
            {
                return;
            }
            // None -> Heap : error
            // Heap -> Heap : 
            pointer first = static_cast<pointer>(AllocatorT::Allocate(sizeof(value_type) * size, alignof(value_type)));
            pointer last = first + size;
            pointer it = first;
            pointer oldIt = mFirst;
            while (it != last)
            {
                it = new(it)value_type();
                *it = *oldIt;
                oldIt->~value_type();

                ++it;
                ++oldIt;
            }
            AllocatorT::Free(mFirst);
            mFirst = first;
            mLast = last;
            mEnd = mLast;
        }

        void Release()
        {
            if (mFirst)
            {
                AllocatorT::Free(mFirst);
            }
            mFirst = mLast = mEnd = nullptr;
        }


        pointer mFirst;
        pointer mLast;
        pointer mEnd;
    };

    template<typename T, SizeT SIZE, typename AllocatorT = DefaultAllocator>
    struct TArrayStaticData
    {
        using value_type = T;
        using pointer = T*;

        TArrayStaticData() :
            mStatic(),
            mFirst(nullptr),
            mLast(nullptr),
            mEnd(nullptr)
        {
            for (SizeT i = 0; i < SIZE; ++i)
            {
                mStatic[i].~value_type();
            }
        }
        TArrayStaticData(TArrayStaticData&& other) :
            mStatic(),
            mFirst(other.mFirst),
            mLast(other.mLast),
            mEnd(other.mEnd)
        {
            for (SizeT i = 0; i < SIZE; ++i)
            {
                mStatic[i] = std::move(other.mStatic[i]);
            }

            if (other.UseStatic())
            {
                SizeT size = other.mLast - other.mFirst;
                SizeT capacity = other.mEnd - other.mFirst;
                mFirst = mStatic;
                mLast = mFirst + size;
                mEnd = mFirst + capacity;
            }
            other.mFirst = nullptr;
            other.mLast = nullptr;
            other.mEnd = nullptr;
        }

        TArrayStaticData& operator=(TArrayStaticData&& other)
        {
            if (other.UseStatic())
            {
                SizeT size = other.mLast - other.mFirst;
                SizeT capacity = other.mEnd - other.mFirst;
                mFirst = mStatic;
                mLast = mFirst + size;
                mEnd = mFirst + capacity;
            }
            else
            {
                mFirst = other.mFirst;
                mLast = other.mLast;
                mEnd = other.mEnd;
            }
            for (SizeT i = 0; i < SIZE; ++i)
            {
                mStatic[i] = std::move(other.mStatic[i]);
            }
            other.mFirst = nullptr;
            other.mLast = nullptr;
            other.mEnd = nullptr;
            return *this;
        }

        void swap(TArrayStaticData& other)
        {
            std::swap(mStatic, other.mStatic);
            std::swap(mFirst, other.mFirst);
            std::swap(mLast, other.mLast);
            std::swap(mEnd, other.mEnd);
        }

        void Grow(SizeT size, SizeT reserve = 2)
        {
            const SizeT capacity = mEnd - mFirst;
            if (size <= capacity)
            {
                return;
            }

            size *= reserve; // grow for extra
            const SizeT oldSize = mLast - mFirst;
            // Cases:
            // 1. None > Static
            // 2. None > Heap
            // 3. Static > Static
            // 4. Static > Heap
            // 5. Heap > Heap

            // None = No Copy
            if (capacity == 0)
            {
                if (size > SIZE)
                {
                    mFirst = static_cast<pointer>(AllocatorT::Allocate(sizeof(value_type) * size, alignof(value_type)));
                    mLast = mFirst;
                    mEnd = mFirst + size;
                }
                else
                {
                    mFirst = mStatic;
                    mLast = mFirst;
                    mEnd = mFirst + SIZE;
                }
                return;
            }

            if (capacity <= SIZE)
            {
                // static to static: no copy or alloc
                if (size < SIZE)
                {
                    mEnd = mFirst + size;
                    return;
                }
                // static to heap:
                pointer first = static_cast<pointer>(AllocatorT::Allocate(sizeof(value_type) * size, alignof(value_type)));
                pointer last = first + oldSize;
                pointer end = first + size;
                pointer it = first;
                pointer oldIt = mFirst;
                while (it != last)
                {
                    it = new(it)value_type();
                    *it = *oldIt;
                    oldIt->~value_type();
                    ++it;
                    ++oldIt;
                }
                it = first;
                mFirst = first;
                mLast = last;
                mEnd = end;
            }
            else
            {
                // heap to heap:
                pointer first = static_cast<pointer>(AllocatorT::Allocate(sizeof(value_type) * size, alignof(value_type)));
                pointer last = first + oldSize;
                pointer end = first + size;
                pointer it = first;
                pointer oldIt = mFirst;
                while (it != last)
                {
                    it = new(it)value_type();
                    *it = *oldIt;
                    oldIt->~value_type();
                    ++it;
                    ++oldIt;
                }
                AllocatorT::Free(mFirst);
                it = first;
                mFirst = first;
                mLast = last;
                mEnd = end;
            }
        }

        void Shrink(SizeT size)
        {
            SizeT capacity = mEnd - mFirst;
            Assert(size <= capacity);
            if (capacity <= size)
            {
                return;
            }
            // None -> Static : error
            // None -> Heap : error
            // Static -> Static
            // Static -> Heap : error
            // Heap -> Heap : 
            // Heap -> Static :


            // Static to heap:
            if (capacity < SIZE && size >= SIZE)
            {
                Crash("Invalid operation, cannot shrink from static TO heap.", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
                return;
            }

            // Static to static
            if (capacity < SIZE && size < SIZE)
            {
                mEnd = mFirst + size;
                return;
            }

            pointer first = nullptr;
            // Heap to Static:
            if (size < SIZE)
            {
                first = mStatic;
            }
            // Heap to Heap:
            else
            {
                first = static_cast<pointer>(AllocatorT::Allocate(sizeof(value_type) * size, alignof(value_type)));
            }

            pointer last = first + size;
            pointer it = first;
            pointer oldIt = mFirst;
            while (it != last)
            {
                it = new(it)value_type();
                *it = *oldIt;
                oldIt->~value_type();

                ++it;
                ++oldIt;
            }
            AllocatorT::Free(mFirst);
            mFirst = first;
            mLast = last;
            mEnd = mLast;
        }

        void Release()
        {
            SizeT capacity = mEnd - mFirst;
            if (capacity > SIZE)
            {
                AllocatorT::Free(mFirst);
            }
            mFirst = mLast = mEnd = nullptr;
        }

        bool UseStatic() const
        {
            return (mEnd - mFirst) <= SIZE;
        }

        value_type mStatic[SIZE];
        pointer mFirst;
        pointer mLast;
        pointer mEnd;
    };

    template<typename T, typename DataT = TArrayData<T>>
    class TArray;


    template<typename T>
    class ArrayConstIterator
    {
    public:
        using container_type = void;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator_category = std::random_access_iterator_tag;
#if defined(LF_ITERATOR_STL_CHECK)
        using checked_type = T*;
        inline checked_type GetUnchecked() { return mPtr; }
        inline ArrayConstIterator& GetRechecked(ArrayConstIterator&, checked_type data) { mPtr = data; return *this; }
#endif

        ArrayConstIterator() :
            mPtr(nullptr),
            mContainer(nullptr)
        {}
        ArrayConstIterator(const ArrayConstIterator& other) :
            mPtr(other.mPtr),
            mContainer(other.mContainer)
        {}
        explicit ArrayConstIterator(pointer ptr, const container_type* container) :
            mPtr(ptr),
            mContainer(container)
        {

        }
        ~ArrayConstIterator()
        {

        }

        ArrayConstIterator& operator=(const ArrayConstIterator& other)
        {
            mPtr = other.mPtr;
            mContainer = other.mContainer;
            return *this;
        }

        void swap(ArrayConstIterator& other)
        {
            std::swap(mPtr, other.mPtr);
            std::swap(mContainer, other.mContainer);
        }

        bool operator!=(const ArrayConstIterator& other) const
        {
            return mPtr != other.mPtr;
        }
        bool operator==(const ArrayConstIterator& other) const
        {
            return mPtr == other.mPtr;
        }

        const_reference operator*() const
        {
            return *mPtr;
        }

        const_pointer operator->() const
        {
            return mPtr;
        }

        const_reference operator[](difference_type n) const
        {
            return mPtr[n];
        }

        // pre-increment
        ArrayConstIterator& operator++()
        {
            ++mPtr;
            return *this;
        }
        // pre-decrement
        ArrayConstIterator& operator--()
        {
            --mPtr;
            return *this;
        }
        // post-increment
        ArrayConstIterator operator++(int)
        {
            ArrayConstIterator tmp(*this);
            ++tmp;
            return tmp;
        }
        // post-decrement
        ArrayConstIterator operator--(int)
        {
            ArrayConstIterator tmp(*this);
            --tmp;
            return tmp;
        }
        ArrayConstIterator&  operator+=(difference_type n)
        {
            mPtr += n;
            return *this;
        }
        ArrayConstIterator   operator+(difference_type n) const
        {
            ArrayConstIterator tmp(*this);
            tmp += n;
            return tmp;
        }
        ArrayConstIterator&  operator-=(difference_type n)
        {
            mPtr -= n;
            return *this;
        }
        ArrayConstIterator   operator-(difference_type n) const
        {
            ArrayConstIterator tmp(*this);
            tmp -= n;
            return tmp;
        }

        difference_type operator-(const ArrayConstIterator& other) const
        {
            return mPtr - other.mPtr;
        }

        bool operator<(const ArrayConstIterator& it) const
        {
            return mPtr < it.mPtr;
        }
        bool operator>(const ArrayConstIterator& it) const
        {
            return mPtr > it.mPtr;
        }
        bool operator>=(const ArrayConstIterator& it) const
        {
            return mPtr >= it.mPtr;
        }
        bool operator<=(const ArrayConstIterator& it) const
        {
            return mPtr <= it.mPtr;
        }

#if defined(LF_ITERATOR_CONTAINER_CHECK)
        const container_type* GetContainer() const
        {
            return mContainer;
        }
#endif
        pointer GetItem()
        {
            return mPtr;
        }

    protected:
        pointer mPtr;
#if defined(LF_ITERATOR_RANGE_CHECK) || defined(LF_ITERATOR_CONTAINER_CHECK)
        const container_type* mContainer;
#endif
    };

    template<typename T>
    class ArrayIterator : public ArrayConstIterator<T>
    {
    public:
        using container_type = void;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator_category = std::random_access_iterator_tag;
        using super = ArrayConstIterator<T>;
#if defined(LF_ITERATOR_STL_CHECK)
        using checked_type = T*;
        inline checked_type GetUnchecked() { return mPtr; }
        inline ArrayIterator& GetRechecked(ArrayIterator&, checked_type data) { mPtr = data; return *this; }
#endif
        ArrayIterator() : super()
        {}
        ArrayIterator(const ArrayIterator& other) : super(other)
        {}
        explicit ArrayIterator(pointer ptr, const container_type* container) : super(ptr, container)
        {}
        ~ArrayIterator()
        {}

        void Set(pointer ptr, container_type* container)
        {
            mPtr = ptr;
            mContainer = container;
        }

        reference operator*()
        {
            return *mPtr;
        }
        pointer operator->()
        {
            return mPtr;
        }
        reference operator[](difference_type n)
        {
            return mPtr[n];
        }

        // pre-increment
        ArrayIterator& operator++()
        {
            ++mPtr;
            return *this;
        }
        // pre-decrement
        ArrayIterator& operator--()
        {
            --mPtr;
            return *this;
        }
        // post-increment
        ArrayIterator operator++(int)
        {
            ArrayIterator tmp(*this);
            ++tmp;
            return tmp;
        }
        // post-decrement
        ArrayIterator operator--(int)
        {
            ArrayIterator tmp(*this);
            --tmp;
            return tmp;
        }
        ArrayIterator&  operator+=(difference_type n)
        {
            mPtr += n;
            return *this;
        }
        ArrayIterator   operator+(difference_type n) const
        {
            ArrayIterator tmp(*this);
            tmp += n;
            return tmp;
        }
        ArrayIterator&  operator-=(difference_type n)
        {
            mPtr -= n;
            return *this;
        }
        ArrayIterator   operator-(difference_type n) const
        {
            ArrayIterator tmp(*this);
            tmp -= n;
            return tmp;
        }

        difference_type operator-(const ArrayIterator& other) const
        {
            return mPtr - other.mPtr;
        }
    };

    template<typename T>
    class ArrayConstReverseIterator
    {
    public:
        using container_type = void;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator_category = std::random_access_iterator_tag;

        ArrayConstReverseIterator() :
            mPtr(nullptr),
            mContainer(nullptr)
        {}
        ArrayConstReverseIterator(const ArrayConstReverseIterator& other) :
            mPtr(other.mPtr),
            mContainer(other.mContainer)
        {}
        explicit ArrayConstReverseIterator(pointer ptr, const container_type* container) :
            mPtr(ptr),
            mContainer(container)
        {

        }
        ~ArrayConstReverseIterator()
        {

        }

        ArrayConstReverseIterator& operator=(const ArrayConstReverseIterator& other)
        {
            mPtr = other.mPtr;
            mContainer = other.mContainer;
            return *this;
        }

        void swap(ArrayConstReverseIterator& other)
        {
            std::swap(mPtr, other.mPtr);
            std::swap(mContainer, other.mContainer);
        }

        bool operator!=(const ArrayConstReverseIterator& other) const
        {
            return mPtr != other.mPtr;
        }
        bool operator==(const ArrayConstReverseIterator& other) const
        {
            return mPtr == other.mPtr;
        }

        const_reference operator*() const
        {
            return *mPtr;
        }

        const_pointer operator->() const
        {
            return mPtr;
        }

        const_reference operator[](difference_type n) const
        {
            return mPtr[n];
        }

        // pre-increment
        ArrayConstReverseIterator& operator++()
        {
            --mPtr;
            return *this;
        }
        // pre-decrement
        ArrayConstReverseIterator& operator--()
        {
            ++mPtr;
            return *this;
        }
        // post-increment
        ArrayConstReverseIterator operator++(int)
        {
            ArrayConstReverseIterator tmp(*this);
            --tmp;
            return tmp;
        }
        // post-decrement
        ArrayConstReverseIterator operator--(int)
        {
            ArrayConstReverseIterator tmp(*this);
            ++tmp;
            return tmp;
        }
        ArrayConstReverseIterator&  operator+=(SizeT n)
        {
            mPtr -= n;
            return *this;
        }
        ArrayConstReverseIterator   operator+(SizeT n) const
        {
            ArrayConstReverseIterator tmp(*this);
            tmp -= n;
            return tmp;
        }
        ArrayConstReverseIterator&  operator-=(SizeT n)
        {
            mPtr += n;
            return *this;
        }
        ArrayConstReverseIterator   operator-(SizeT n) const
        {
            ArrayConstReverseIterator tmp(*this);
            tmp += n;
            return tmp;
        }

        difference_type operator-(const ArrayConstReverseIterator& other) const
        {
            return other.mPtr - mPtr;
        }

        bool operator<(const ArrayConstReverseIterator& it) const
        {
            return mPtr > it.mPtr;
        }
        bool operator>(const ArrayConstReverseIterator& it) const
        {
            return mPtr < it.mPtr;
        }
        bool operator>=(const ArrayConstReverseIterator& it) const
        {
            return mPtr <= it.mPtr;
        }
        bool operator<=(const ArrayConstReverseIterator& it) const
        {
            return mPtr >= it.mPtr;
        }

#if defined(LF_ITERATOR_CONTAINER_CHECK)
        const container_type* GetContainer() const
        {
            return mContainer;
        }
#endif
        pointer GetItem()
        {
            return mPtr;
        }

        ArrayConstIterator<T> GetBase() const
        {
            return ArrayConstIterator<T>(mPtr, mContainer);
        }
    protected:
        pointer mPtr;
#if defined(LF_ITERATOR_RANGE_CHECK) || defined(LF_ITERATOR_CONTAINER_CHECK)
        const container_type* mContainer;
#endif
    };

    template<typename T>
    class ArrayReverseIterator : public ArrayConstReverseIterator<T>
    {
    public:
        using container_type = void;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator_category = std::random_access_iterator_tag;
        using super = ArrayConstReverseIterator<T>;

        ArrayReverseIterator() : super()
        {}
        ArrayReverseIterator(const ArrayReverseIterator& other) : super(other)
        {}
        explicit ArrayReverseIterator(pointer ptr, const container_type* container) : super(ptr, container)
        {}
        ~ArrayReverseIterator()
        {}

        reference operator*()
        {
            return *mPtr;
        }
        pointer operator->()
        {
            return mPtr;
        }
        reference operator[](difference_type n)
        {
            return mPtr[n];
        }

        // pre-increment
        ArrayReverseIterator& operator++()
        {
            --mPtr;
            return *this;
        }
        // pre-decrement
        ArrayReverseIterator& operator--()
        {
            ++mPtr;
            return *this;
        }
        // post-increment
        ArrayReverseIterator operator++(int)
        {
            ArrayReverseIterator tmp(*this);
            --tmp;
            return tmp;
        }
        // post-decrement
        ArrayReverseIterator operator--(int)
        {
            ArrayReverseIterator tmp(*this);
            ++tmp;
            return tmp;
        }
        ArrayReverseIterator&  operator+=(SizeT n)
        {
            mPtr -= n;
            return *this;
        }
        ArrayReverseIterator   operator+(SizeT n) const
        {
            ArrayReverseIterator tmp(*this);
            tmp -= n;
            return tmp;
        }
        ArrayReverseIterator&  operator-=(SizeT n)
        {
            mPtr += n;
            return *this;
        }
        ArrayReverseIterator   operator-(SizeT n) const
        {
            ArrayReverseIterator tmp(*this);
            tmp += n;
            return tmp;
        }

        difference_type operator-(const ArrayReverseIterator& other) const
        {
            return other.mPtr - mPtr;
        }

        ArrayIterator<T> GetBase()
        {
            return ArrayIterator<T>(mPtr, mContainer);
        }
    };

    template<typename T, typename DataT>
    class TArray
    {
    public:
        using value_type = T;
        using difference_type = ptrdiff_t;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;
        using iterator = ArrayIterator<T>;
        using const_iterator = ArrayConstIterator<T>;
        using reverse_iterator = ArrayReverseIterator<T>;
        using const_reverse_iterator = ArrayConstReverseIterator<T>;

        TArray() :
            mData()
        {}
        TArray(const TArray& other) :
            mData()
        {
            Insert(end(), other.begin(), other.end());
        }
        TArray(TArray&& other) :
            mData(std::move(other.mData))
        {

        }
        TArray(const std::initializer_list<T>& items) :
            mData()
        {
            Insert(end(), items.begin(), items.end());
        }
        template<typename IteratorT>
        TArray(IteratorT first, IteratorT last) :
            mData()
        {
            Insert(end(), first, last);
        }

        ~TArray()
        {
            Clear();
        }

        iterator begin() { return iterator(mData.mFirst, this); }
        iterator end() { return iterator(mData.mLast, this); }
        const_iterator begin() const { return const_iterator(mData.mFirst, this); }
        const_iterator end() const { return const_iterator(mData.mLast, this); }
        const_iterator cbegin() const { return const_iterator(mData.mFirst, this); }
        const_iterator cend() const { return const_iterator(mData.mLast, this); }

        reverse_iterator rbegin() { return reverse_iterator(mData.mLast - 1, this); }
        reverse_iterator rend() { return reverse_iterator(mData.mFirst - 1, this); }
        const_reverse_iterator rbegin() const { return const_reverse_iterator(mData.mLast - 1, this); }
        const_reverse_iterator rend() const { return const_reverse_iterator(mData.mFirst - 1, this); }

        void swap(TArray& other)
        {
            std::swap(mData, other.mData);
        }

        bool  Empty() const { return mData.mFirst == mData.mLast; }
        SizeT Size() const { return mData.mLast - mData.mFirst; }
        SizeT Capacity() const { return mData.mEnd - mData.mFirst; }


        // **********************************
        // Resizes the array either growing or shrinking it depending on the desired size and current size.
        // -Shrinking will invoke the destructors on the elements that have been destroyed.
        // -Growing will invoke the default constructors on the newly added elements.
        // -Shrinking does NOT release memory call Clear to do that.
        // **********************************
        void Resize(const SizeT size)
        {
            const SizeT currentSize = Size();
            if (currentSize == size)
            {
                return;
            }
            else if (size < currentSize)
            {
                // Shrink:
                pointer last = mData.mFirst + (currentSize - 1);
                pointer itEnd = mData.mFirst + size;
                if (last == itEnd)
                {
                    Destroy(last);
                }
                else
                {
                    do
                    {
                        Destroy(last);
                        --last;
                    } while (last != itEnd);
                    Destroy(last);
                }
                mData.mLast = itEnd;
            }
            else
            {
                // Grow:
                const SizeT capacity = Capacity();
                if (size <= capacity)
                {
                    pointer it = mData.mFirst + currentSize;
                    mData.mLast = mData.mFirst + size;
                    while (it != mData.mLast)
                    {
                        Construct(it);
                        ++it;
                    }
                }
                else
                {
                    // Construct from current size to new size
                    Grow(size);
                    mData.mLast = mData.mFirst + size;
                    pointer it = mData.mFirst + currentSize;
                    while (it != mData.mLast)
                    {
                        Construct(it);
                        ++it;
                    }
                }
            }

        }

        // **********************************
        // Reserve can only allocate MORE space for items it cannot be used to shrink. Use Resize/Collapse for that.
        // - Current size / items are unaffected except for being copied to the newly allocated space.
        // **********************************
        void Reserve(const SizeT capacity)
        {
            const SizeT size = Size();
            Grow(capacity, 1);
            mData.mLast = mData.mFirst + size;
        }

        // **********************************
        // Collapse reduces the memory that the array uses by deallocating the reserved memory.
        // **********************************
        void Collapse()
        {
            const SizeT size = Size();
            if (size == 0)
            {
                Clear();
                return;
            }

            mData.Shrink(size);
            Assert(Size() == size);
        }

        // **********************************
        // Clear will deallocate all memory used by the array. The size and capacity are 0 afterwards.
        // **********************************
        void Clear()
        {
            if (mData.mFirst)
            {
                // destroy all elements
                pointer it = mData.mFirst;
                while (it != mData.mLast)
                {
                    Destroy(it);
                    ++it;
                }
                mData.Release();
            }
        }

        // 0 == front
        // end == back
        // middle
        void Insert(iterator position, const value_type& item)
        {
#if defined(LF_ITERATOR_CONTAINER_CHECK)
            if (position.GetContainer() != reinterpret_cast<void*>(this))
            {
                Crash("Iterator container mismatch!", LF_ERROR_BAD_STATE, ERROR_API_CORE);
            }
#endif
#if defined(LF_ITERATOR_RANGE_CHECK)
            if (!(position.GetItem() >= mData.mFirst && position.GetItem() <= mData.mLast))
            {
                Crash("Iterator out of range!", LF_ERROR_OUT_OF_RANGE, ERROR_API_CORE);
            }
#endif
            if (position.GetItem() == mData.mEnd)
            {
                Add(item);
                return;
            }
            const SizeT newSize = Size() + 1;
            bool hasSpace = newSize <= Capacity();
            const SizeT index = position.GetItem() - mData.mFirst;

            if (!hasSpace)
            {
                Grow(newSize);
                mData.mLast = mData.mFirst + newSize;
            }
            else
            {
                ++mData.mLast;
            }
            Construct(mData.mLast - 1);
            position = begin() + index;

            pointer ptr = mData.mLast - 2;
            pointer next = ptr + 1;
            while (next != position.GetItem())
            {
                *next = *ptr;
                --next;
                --ptr;
            }

            *position.GetItem() = item;
        }

        template<typename IteratorT>
        void Insert(iterator position, IteratorT first, IteratorT last)
        {
#if defined(LF_ITERATOR_CONTAINER_CHECK)
            if (position.GetContainer() != reinterpret_cast<void*>(this))
            {
                Crash("Iterator container mismatch!", LF_ERROR_BAD_STATE, ERROR_API_CORE);
            }
#endif
#if defined(LF_ITERATOR_RANGE_CHECK)
            if (!(position.GetItem() >= mData.mFirst && position.GetItem() <= mData.mLast))
            {
                Crash("Iterator out of range!", LF_ERROR_OUT_OF_RANGE, ERROR_API_CORE);
            }
#endif

            const SizeT distance = std::distance(first, last);
            const SizeT offset = position.GetItem() - mData.mFirst;
            const SizeT oldSize = Size();

            const SizeT newSize = oldSize + distance;
            const SizeT moveSize = oldSize - offset;

            // Grow:
            if (newSize > Capacity())
            {
                Grow(newSize);
            }
            mData.mLast = mData.mFirst + newSize;

            // Construct Last to NewLast
            pointer it = mData.mFirst + oldSize;
            while (it != mData.mLast)
            {
                Construct(it);
                ++it;
            }

            // shift right
            pointer itEnd = mData.mLast - 1;
            pointer oldEnd = mData.mFirst + (oldSize - 1);
            for (SizeT i = 0; i < moveSize; ++i)
            {
                *itEnd = *oldEnd;
                --itEnd;
                --oldEnd;
            }

            // insert
            it = mData.mFirst + offset;
            for (SizeT i = 0; i < distance; ++i)
            {
                *it = *first;
                ++it;
                ++first;
            }
        }

        void Add(const value_type& item)
        {
            SizeT index = Size();
            // SizeT capacityBefore = Capacity();
            Grow(index + 1);
            mData.mLast = mData.mFirst + index + 1;
            Construct(&mData.mFirst[index]);
            mData.mFirst[index] = item;
        }

        iterator Remove(iterator it)
        {
#if defined(LF_ITERATOR_CONTAINER_CHECK)
            if (it.GetContainer() != reinterpret_cast<void*>(this))
            {
                Crash("Iterator container mismatch!", LF_ERROR_BAD_STATE, ERROR_API_CORE);
            }
#endif
#if defined(LF_ITERATOR_RANGE_CHECK)
            if (!(it.GetItem() >= mData.mFirst && it.GetItem() <= mData.mLast))
            {
                Crash("Iterator out of range!", LF_ERROR_OUT_OF_RANGE, ERROR_API_CORE);
            }
#endif

            if (it == end())
            {
                return it;
            }

            // copy_shift_left:
            pointer ptr = it.GetItem();
            pointer next = ptr + 1;
            while (next != mData.mLast)
            {
                *ptr = *next;
                ++next;
                ++ptr;
            }
            Destroy(ptr);
            --mData.mLast;
            return it;
        }

        iterator SwapRemove(iterator it)
        {
#if defined(LF_ITERATOR_CONTAINER_CHECK)
            if (it.GetContainer() != reinterpret_cast<void*>(this))
            {
                Crash("Iterator container mismatch!", LF_ERROR_BAD_STATE, ERROR_API_CORE);
            }
#endif
#if defined(LF_ITERATOR_RANGE_CHECK)
            if (!(it.GetItem() >= mData.mFirst && it.GetItem() <= mData.mLast))
            {
                Crash("Iterator out of range!", LF_ERROR_OUT_OF_RANGE, ERROR_API_CORE);
            }
#endif
            if (it == end())
            {
                return it;
            }

            value_type tmp = *it.GetItem();
            reference item = *it.GetItem();
            reference last = GetLast();
            item = last;
            last = tmp;

            Destroy(&last);
            --mData.mLast;

            return it;
        }

        SizeT IndexOf(const value_type& value) const
        {
            if (Empty())
            {
                return INVALID;
            }

            SizeT i = 0;

            pointer it = begin().GetItem();
            pointer last = end().GetItem();
            for (; it != last; ++it)
            {
                if (*it == value)
                {
                    return i;
                }
                ++i;
            }
            return INVALID;
        }
        // 
        // void SwapHeap(sys::GenericHeap* heap);

        reference GetLast()
        {
            Assert(!Empty());
            return mData.mFirst[Size() - 1];
        }

        const_reference GetLast() const
        {
            Assert(!Empty());
            return mData.mFirst[Size() - 1];
        }

        reference GetFirst()
        {
            Assert(!Empty());
            return mData.mFirst[0];
        }

        const_reference GetFirst() const
        {
            Assert(!Empty());
            return mData.mFirst[0];
        }

        reference operator[](difference_type i)
        {
            return mData.mFirst[i];
        }
        const_reference operator[](difference_type i) const
        {
            return mData.mFirst[i];
        }

        TArray& operator=(const TArray& other)
        {
            if (this == &other)
            {
                return *this;
            }
            Clear();
            Insert(end(), other.begin(), other.end());
            return *this;
        }

        TArray& operator=(TArray&& other)
        {
            if (this == &other)
            {
                return *this;
            }
            Clear();
            mData = std::move(other.mData);
            return *this;
        }

        bool operator==(const TArray& other) const
        {
            if (other.Size() != other.Size())
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

        bool operator!=(const TArray& other) const
        {
            return !(*this == other);
        }

        pointer GetData()
        {
            return mData.mFirst;
        }

        const_pointer GetData() const
        {
            return mData.mFirst;
        }
    private:
        void Grow(SizeT size, SizeT reserve = 2)
        {
            mData.Grow(size, reserve);
        }
        void Destroy(pointer ptr)
        {
            ptr->~value_type();
            (ptr);
        }
        void Construct(pointer ptr)
        {
            new(ptr)value_type();
        }

        DataT mData;

    };

    template<typename T, SizeT SIZE>
    using TStaticArray = TArray<T, TArrayStaticData<T, SIZE>>;
}

#if defined(LF_ITERATOR_STL_CHECK)
namespace std
{
    template<typename T>
    inline typename ::lf::ArrayIterator<T>::checked_type
        _Unchecked(::lf::ArrayIterator<T>& iter)
    {
        return iter.GetUnchecked();
    }
    template<typename T>
    inline ::lf::ArrayIterator<T>&
        _Rechecked(::lf::ArrayIterator<T>& iter, typename ::lf::ArrayIterator<T>::checked_type data)
    {
        return iter.GetRechecked(iter, data);
    }
}
#endif

#endif // LF_CORE_ARRAY_H
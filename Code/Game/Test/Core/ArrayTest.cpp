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
// #include "core/util/Array.h"
// #include "core/util/ArrayList.h"
// #include "core/system/SmartPointer.h"

#include "Core/Memory/SmartPointer.h"
#include "Core/String/String.h"
#include "Core/Math/MathFunctions.h"
#include "Core/Math/Vector.h"
#include "Core/Test/Test.h"
#include "Core/Utility/Array.h"
#include "Core/Utility/ArrayList.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Utility.h"

#include <algorithm>

namespace lf
{
    using IntPtr = TStrongPointer<int>;
    static IntPtr MakePtr(int v)
    {
        IntPtr ptr = IntPtr(LFNew<int>());
        *ptr = v;
        return ptr;
    }

    template<typename T>
    static bool CheckReference(const T& collection)
    {
        for (typename T::const_reference item : collection)
        {
            if (item.GetStrongRefs() != 1)
            {
                return false;
            }
        }
        return true;
    }

    template<typename T, SizeT SIZE>
    static bool CheckSequence(const TArray<TStrongPointer<T>>& collection, T sequence[SIZE])
    {
        if (collection.Size() != SIZE)
        {
            return false;
        }
        SizeT index = 0;
        for (typename TArray<TStrongPointer<T>>::const_reference item : collection)
        {
            if ((*item) != sequence[index])
            {
                return false;
            }
            ++index;
        }
        return true;
    }

    template<typename T, SizeT SIZE>
    static bool CheckSequence(const TArray<T>& collection, T sequence[SIZE])
    {
        if (collection.Size() != SIZE)
        {
            return false;
        }
        SizeT index = 0;
        for (typename TArray<T>::const_reference item : collection)
        {
            if ((item) != sequence[index])
            {
                return false;
            }
            ++index;
        }
        return true;
    }

    template<typename T, SizeT SIZE>
    static bool CheckSequence(const TStaticArray<TStrongPointer<T>, 4>& collection, T sequence[SIZE])
    {
        if (collection.Size() != SIZE)
        {
            return false;
        }
        SizeT index = 0;
        for (typename TStaticArray<TStrongPointer<T>, 4>::const_reference item : collection)
        {
            if ((*item) != sequence[index])
            {
                return false;
            }
            ++index;
        }
        return true;
    }

    template<typename T, SizeT SIZE>
    static bool CheckSequence(const TStaticArray<T, 4>& collection, T sequence[SIZE])
    {
        if (collection.Size() != SIZE)
        {
            return false;
        }
        SizeT index = 0;
        for (typename TStaticArray<T, 9>::const_reference item : collection)
        {
            if ((item) != sequence[index])
            {
                return false;
            }
            ++index;
        }
        return true;
    }

    template<typename T, SizeT SIZE>
    static bool CheckSequence(const TArrayList<T, 10>& collection, T sequence[SIZE])
    {
        if (collection.Size() != SIZE)
        {
            return false;
        }
        SizeT index = 0;
        for (typename TArrayList<T, 10>::const_reference item : collection)
        {
            if ((item) != sequence[index])
            {
                return false;
            }
            ++index;
        }
        return true;
    }

    template<typename T, SizeT SIZE>
    static bool CheckSequence(const TArrayList<TStrongPointer<T>, 10>& collection, T sequence[SIZE])
    {
        if (collection.Size() != SIZE)
        {
            return false;
        }
        SizeT index = 0;
        for (typename TArrayList<TStrongPointer<T>, 10>::const_reference item : collection)
        {
            if ((*item) != sequence[index])
            {
                return false;
            }
            ++index;
        }
        return true;
    }

#define TEST_SEQUENCE(Type, Collection, ...) \
    {                             \
        Type INTERNAL_SEQUENCE[] = { __VA_ARGS__ };\
        bool result = CheckSequence<Type, LF_ARRAY_SIZE(INTERNAL_SEQUENCE)>(Collection, INTERNAL_SEQUENCE); \
        TEST(result); \
    } \

    // TEST(CheckSequence<Type, ARRAY_SIZE(INTERNAL_SEQUENCE)>(Collection, INTERNAL_SEQUENCE)); 

    template<typename T>
    static void TestArrayAddRemove()
    {
        T a;
        SizeT used = LFGetBytesAllocated();
        
        TEST(a.Empty());
        TEST(a.Size() == 0);
        TEST(a.Capacity() == 0);
        a.Add(MakePtr(5));
        TEST(a.Size() == 1);
        TEST(a.Capacity() >= 1);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 5);
        CheckReference(a);
        a.Add(MakePtr(7));
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 5, 7);
        CheckReference(a);
        a.Add(MakePtr(3));
        TEST(a.Size() == 3);
        TEST(a.Capacity() >= 3);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 5, 7, 3);
        CheckReference(a);
        // Remove First:
        a.Remove(a.begin());
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 7, 3);
        CheckReference(a);
        a.Add(MakePtr(5));
        TEST(a.Size() == 3);
        TEST(a.Capacity() >= 3);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 7, 3, 5);
        CheckReference(a);
        // Remove Middle:
        a.Remove(a.begin() + 1);
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 7, 5);
        CheckReference(a);
        a.Add(MakePtr(3));
        TEST(a.Size() == 3);
        TEST(a.Capacity() >= 3);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 7, 5, 3);
        CheckReference(a);
        // Remove Last:
        a.Remove(a.begin() + 2);
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 7, 5);
        CheckReference(a);
        a.Clear();

        TEST(used == LFGetBytesAllocated());
    }

    template<typename T>
    static void TestArraySwapRemove()
    {
        T a;
        SizeT used = LFGetBytesAllocated();

        TEST(a.Empty());
        TEST(a.Size() == 0);
        TEST(a.Capacity() == 0);
        a.Add(MakePtr(5));
        TEST(a.Size() == 1);
        TEST(a.Capacity() >= 1);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 5);
        CheckReference(a);
        a.Add(MakePtr(7));
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 5, 7);
        CheckReference(a);
        a.Add(MakePtr(3));
        TEST(a.Size() == 3);
        TEST(a.Capacity() >= 3);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 5, 7, 3);
        CheckReference(a);
        // Remove First:
        a.SwapRemove(a.begin());
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 3, 7);
        CheckReference(a);
        a.Add(MakePtr(5));
        TEST(a.Size() == 3);
        TEST(a.Capacity() >= 3);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 3, 7, 5);
        CheckReference(a);
        // Remove Middle:
        a.SwapRemove(a.begin() + 1);
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 3, 5);
        CheckReference(a);
        a.Add(MakePtr(7));
        TEST(a.Size() == 3);
        TEST(a.Capacity() >= 3);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 3, 5, 7);
        CheckReference(a);
        // Remove Last:
        a.SwapRemove(a.begin() + 2);
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 3, 5);
        CheckReference(a);
        a.Clear();

        TEST(used == LFGetBytesAllocated());
    }

    template<typename T>
    static void TestArrayResize()
    {
        T a;
        IntPtr x = MakePtr(30);
        IntPtr y = MakePtr(42);

        SizeT used = LFGetBytesAllocated();
        TEST(a.Empty());
        TEST(a.Size() == 0);
        TEST(a.Capacity() == 0);
        // Do nothing:
        a.Resize(0);
        TEST(a.Empty());
        TEST(a.Size() == 0);
        TEST(a.Capacity() == 0);

        // Grow:
        a.Resize(3);
        TEST(!a.Empty());
        TEST(a.Size() == 3);
        TEST(a.Capacity() >= 3);
        for (IntPtr& item : a)
        {
            TEST(item == NULL_PTR);
        }
        a[0] = x;
        a[2] = y;

        // Keep Capacity:
        SizeT capacityBefore = a.Capacity();
        a.Resize(5);
        TEST(capacityBefore == a.Capacity());
        TEST(!a.Empty());
        TEST(a.Size() == 5);
        TEST(a.Capacity() >= 5);
        TEST(a[0] == x);
        TEST(a[2] == y);

        // Grow:
        a.Resize(7);
        TEST(!a.Empty());
        TEST(a.Size() == 7);
        TEST(a.Capacity() >= 7);
        TEST(a[0] == x);
        TEST(a[2] == y);

        // Shrink:
        a.Resize(2);
        TEST(!a.Empty());
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST(a[0] == x);

        TEST(x.GetStrongRefs() == 2);
        TEST(y.GetStrongRefs() == 1);

        a.Clear();
        TEST(used == LFGetBytesAllocated());
    }

    template<typename T>
    static void TestArrayReserve(SizeT staticSize)
    {
        T a;
        IntPtr x = MakePtr(30);
        IntPtr y = MakePtr(42);

        SizeT used = LFGetBytesAllocated();
        TEST(a.Empty());
        TEST(a.Size() == 0);
        TEST(a.Capacity() == 0);

        a.Reserve(3);
        TEST(a.Empty());
        TEST(a.Size() == 0);
        TEST(a.Capacity() == Max<SizeT>(3, staticSize));

        a.Add(x);
        a.Add(NULL_PTR);
        a.Add(y);

        a.Reserve(6);
        TEST(!a.Empty());
        TEST(a.Size() == 3);
        TEST(a.Capacity() == 6);

        a.Clear();
        TEST(used == LFGetBytesAllocated());
    }

    template<typename T>
    static void TestCollapse()
    {
        T a;
        IntPtr x = MakePtr(30);
        IntPtr y = MakePtr(42);
        SizeT used = LFGetBytesAllocated();

        a.Reserve(6);
        TEST(a.Empty());
        TEST(a.Size() == 0);
        TEST(a.Capacity() == 6);


        a.Add(x);
        a.Add(NULL_PTR);
        a.Add(y);
        a.Collapse();
        TEST(!a.Empty());
        TEST(a.Size() == 3);
        TEST(a.Capacity() == 3);
        TEST(a[0] == x);
        TEST(a[1] == NULL_PTR);
        TEST(a[2] == y);

        a.SwapRemove(a.begin());
        a.SwapRemove(a.begin());
        a.SwapRemove(a.begin());
        TEST(a.Empty());
        TEST(a.Size() == 0);
        a.Collapse();
        TEST(a.Empty());
        TEST(a.Capacity() == 0);
        TEST(a.Size() == 0);

        a.Clear();
        TEST(used == LFGetBytesAllocated());
    }

    template<typename T>
    static void TestInsert()
    {
        T a;
        IntPtr x = MakePtr(30);
        IntPtr y = MakePtr(42);
        SizeT used = LFGetBytesAllocated();

        a.Insert(a.begin(), x);
        TEST(a.Size() == 1);
        TEST(a.Capacity() >= 1);
        TEST_SEQUENCE(int, a, 30);
        TEST(x.GetStrongRefs() == 2);

        a.Insert(a.begin(), y);
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST_SEQUENCE(int, a, 42, 30);
        TEST(x.GetStrongRefs() == 2);
        TEST(y.GetStrongRefs() == 2);

        a.Insert(a.begin() + 1, MakePtr(7));
        TEST(a.Size() == 3);
        TEST(a.Capacity() >= 3);
        TEST_SEQUENCE(int, a, 42, 7, 30);
        TEST(a[1].GetStrongRefs() == 1);
        TEST(x.GetStrongRefs() == 2);
        TEST(y.GetStrongRefs() == 2);

        a.Insert(a.begin(), MakePtr(50));
        TEST(a.Size() == 4);
        TEST(a.Capacity() >= 4);
        TEST_SEQUENCE(int, a, 50, 42, 7, 30);
        TEST(a[0].GetStrongRefs() == 1);
        TEST(a[2].GetStrongRefs() == 1);
        TEST(x.GetStrongRefs() == 2);
        TEST(y.GetStrongRefs() == 2);

        a.Insert(a.end(), MakePtr(150));
        TEST(a.Size() == 5);
        TEST(a.Capacity() >= 5);
        TEST_SEQUENCE(int, a, 50, 42, 7, 30, 150);

        a.Insert(a.end() - 1, MakePtr(200));
        TEST(a.Size() == 6);
        TEST(a.Capacity() >= 6);
        TEST_SEQUENCE(int, a, 50, 42, 7, 30, 200, 150);

        a.Clear();
        TEST(used == LFGetBytesAllocated());

        a.Reserve(8);
        a.Add(MakePtr(0));
        a.Add(MakePtr(1));
        a.Add(MakePtr(2));
        a.Add(MakePtr(3));
        a.Add(MakePtr(4));
        a.Add(MakePtr(5));
        a.Add(MakePtr(6));
        a.Add(MakePtr(7));

        TArray<IntPtr> b;
        b.Add(MakePtr(10));
        b.Add(MakePtr(11));
        b.Add(MakePtr(12));

        a.Insert(a.begin() + 3, b.begin(), b.end());
        TEST_SEQUENCE(int, a, 0, 1, 2, 10, 11, 12, 3, 4, 5, 6, 7);

        b.Clear();
        a.Clear();
        TEST(used == LFGetBytesAllocated());
    }

    template<typename T>
    static void TestArrayConstructors()
    {
        SizeT used = LFGetBytesAllocated();
        // Default Ctor
        {
            T a;
            TEST(a.Empty());
            TEST(a.Capacity() == 0);
        }

        // Initializer List Ctor
        {
            T a = { MakePtr(30), MakePtr(85), MakePtr(100) };
            TEST(a.Size() == 3);
            TEST(a.Capacity() >= 3);
            TEST_SEQUENCE(int, a, 30, 85, 100);
        }

        // Iterator Ctor
        {
            T items = { MakePtr(30), MakePtr(85), MakePtr(100) };
            T a(items.begin(), items.end());
            TEST(a.Size() == 3);
            TEST(a.Capacity() >= 3);
            TEST_SEQUENCE(int, a, 30, 85, 100);
        }
        TEST(used == LFGetBytesAllocated());
    }

    template<typename T>
    static void TestArrayReverseIterator()
    {
        T a;
        SizeT used = LFGetBytesAllocated();

        a.Add(MakePtr(0));
        a.Add(MakePtr(1));
        a.Add(MakePtr(2));
        a.Add(MakePtr(3));
        a.Add(MakePtr(4));

        {
            TArray<IntPtr> b(a.rbegin(), a.rend());
            TEST_SEQUENCE(int, b, 4, 3, 2, 1, 0);
        }
        a.Clear();

        TEST(used == LFGetBytesAllocated());
    }

    template<typename T>
    static void TestSorting()
    {
        // Unstable:
        SizeT used = LFGetBytesAllocated();
        {
            T a =
            {
                (69),
                (68),
                (70),
                (67),
                (71),
                (66)
            };

            std::sort(a.begin(), a.end());
            TEST_SEQUENCE(int, a, 66, 67, 68, 69, 70, 71);
        }
        TEST(used == LFGetBytesAllocated());
        {
            T a =
            {
                (69),
                (68),
                (66),
                (70),
                (67),
                (71),
                (70),
                (66)
            };

            std::stable_sort(a.begin(), a.end());
            TEST_SEQUENCE(int, a, 66, 66, 67, 68, 69, 70, 70, 71);
        }
        TEST(used == LFGetBytesAllocated());
    }

    // This was a bug where inserting elements into a static array would result in an allocation
    // when in fact the static allocation would've sufficed.
    static void TestInsertionBug()
    {
        SizeT bytesUsed = LFGetBytesAllocated();
        {
            TStaticArray<Float32, 30> items;
            {
                TStaticArray<Float32, 30> copy;
                for (SizeT i = 0; i < 30; ++i)
                {
                    items.Add(static_cast<Float32>(i));
                    copy.Insert(copy.end(), items.begin(), items.end());
                    items.SwapRemove(items.begin());
                }
            }
        }
        SizeT bytesAfter = LFGetBytesAllocated();
        TEST(bytesUsed == bytesAfter);
    }

    template<typename T>
    static void TestArrayEquality()
    {
        T a = { 5, 7, 9, 6, 3, 5, 4 };
        T b = { 7, 4, 3, 2, 3, 6, 5 };
        T c = { 5, 7, 9 };

        T aCopy = a;

        TEST(a == aCopy);
        TEST(!(a == b));
        TEST(a != b);
        TEST(a != c);
    }

    REGISTER_TEST(ArrayTest)
    {
        TestArrayAddRemove<TArray<IntPtr>>();
        TestArraySwapRemove<TArray<IntPtr>>();
        TestArrayResize<TArray<IntPtr>>();
        TestArrayReserve<TArray<IntPtr>>(0);
        TestCollapse<TArray<IntPtr>>();
        TestInsert<TArray<IntPtr>>();
        TestArrayConstructors<TArray<IntPtr>>();
        TestArrayReverseIterator<TArray<IntPtr>>();
        TestSorting<TArray<int>>();
        TestArrayEquality<TArray<int>>();

        TestArrayAddRemove<TStaticArray<IntPtr, 4>>();
        TestArraySwapRemove<TStaticArray<IntPtr, 4>>();
        TestArrayResize<TStaticArray<IntPtr, 4>>();
        TestArrayReserve<TStaticArray<IntPtr, 4>>(4);
        TestCollapse<TStaticArray<IntPtr, 4>>();
        TestInsert<TStaticArray<IntPtr, 4>>();
        TestArrayConstructors<TStaticArray<IntPtr, 4>>();
        TestArrayReverseIterator<TStaticArray<IntPtr, 4>>();
        TestSorting<TStaticArray<int, 4>>();
        TestArrayEquality<TStaticArray<int, 4>>();

        TestInsertionBug();
    }

    static void ArrayListUtilTest()
    {
        // Test element to block formula:
        // TEST(internal_util::ElementToBlockCount(710, 20) == 36);
        // TEST(internal_util::ElementToBlockCount(720, 20) == 36);
        // TEST(internal_util::ElementToBlockCount(700, 20) == 35);
        // TEST(internal_util::ElementToBlockCount(690, 20) == 35);
        // TEST(internal_util::ElementToBlockCount(3, 10) == 1);
    }


    template<typename ArrayListT, typename IteratorT>
    static void ArrayListIteratorTest()
    {
        using block_type = typename ArrayListT::block_type;
        const SizeT offset = offsetof(block_type, mItems);

        block_type a;
        block_type b;
        block_type c;

        a.mState.mNext = &b.mState;
        b.mState.mPrevious = &a.mState;
        b.mState.mNext = &c.mState;
        c.mState.mPrevious = &b.mState;

        a.mState.mItemMask = 0xB; // U U F U
        a.mItems[0] = 1;
        a.mItems[1] = 2;
        a.mItems[2] = INVALID32;
        a.mItems[3] = 3;

        b.mState.mItemMask = 0x9; // U F F U
        b.mItems[0] = 4;
        b.mItems[1] = INVALID32;
        b.mItems[2] = INVALID32;
        b.mItems[3] = 5;

        c.mState.mItemMask = 0xCF; // U F F U F F U U
        c.mItems[0] = 6;
        c.mItems[1] = 7;
        c.mItems[2] = 8;
        c.mItems[3] = 9;
        c.mItems[4] = INVALID32;
        c.mItems[5] = INVALID32;
        c.mItems[6] = 10;
        c.mItems[7] = 11;

        using iterator_type = IteratorT;
        iterator_type it(&a.mState, 0, offset, block_type::BLOCK_SIZE, nullptr);

        for (SizeT i = 1; i < 12; ++i)
        {
            int value = *it;
            TEST(value == i);
            ++it;
        }

        for (SizeT i = 11; i != 0; --i)
        {
            --it;
            int value = *it;
            TEST(value == i);
        }

        --it;

        for (SizeT i = 1; i < 12; ++i)
        {
            ++it;
            int value = *it;
            TEST(value == i);
        }

        it = iterator_type(&a.mState, 0, offset, block_type::BLOCK_SIZE, nullptr);
        {
            it += 3;
            int value = *it;
            TEST(value == 4);
            TEST(it.GetElementIndex() == 3);
        }


        iterator_type other(it - 3);
        {
            int value = *other;
            TEST(value == 1);
            TEST(other.GetElementIndex() == 0);
        }

        iterator_type::difference_type diffAB = it - other;
        iterator_type::difference_type diffBA = other - it;
        TEST(diffAB != diffBA);
        TEST(Abs(diffBA) == diffAB);

        gTestLog.Info(LogMessage("sizeof(ArrayListIterator<int>) == ") << sizeof(iterator_type));

    }


    static void ArrayListSubTest(const SizeT size)
    {
        using container_type = TArrayList<int, 10>;
        using iterator_type = typename container_type::iterator;
        container_type container;
        TEST(container.GetBlockCount() == 0);
        TEST(container.Size() == 0);
        TEST(container.Capacity() == 0);

        iterator_type it = container.Add(5);
        TEST(container.GetBlockCount() == 1);
        TEST(container.Size() == 1);
        TEST(container.Capacity() == size);
        TEST(*it == 5);

        it = container.Add(7);
        TEST(container.GetBlockCount() == 1);
        TEST(container.Size() == 2);
        TEST(container.Capacity() == size);
        TEST(*it == 7);

        container.Clear();

        TEST(container.GetBlockCount() == 0);
        TEST(container.Size() == 0);
        TEST(container.Capacity() == 0);
        container.Reserve(25);
        TEST(container.Capacity() == 3 * size);
        container.Reserve(45);
        TEST(container.Capacity() == 5 * size);
        container.Reserve(28);
        TEST(container.Capacity() == 5 * size);


        it = container.begin();
        iterator_type end = container.end();
        TEST(it == end);

    }

    template<SizeT SIZE>
    static void ArrayListAddRemove()
    {
        // simple numbers:
        SizeT used = LFGetBytesAllocated();
        {
            using list_type = TArrayList<int, SIZE>;
            using iterator_type = typename list_type::iterator;

            {
                list_type l;
                TEST(l.Size() == 0);
                TEST(l.Capacity() == 0);
                iterator_type it = l.Add(5);
                TEST(l.Size() == 1);
                TEST(l.Capacity() == SIZE);
                TEST(it == l.begin());
                it = l.Remove(it);
                TEST(it == l.end());
                TEST(l.begin() == l.end());
                TEST(l.Size() == 0);
                TEST(l.Capacity() == 0);
                l.Clear();
            }
        }

        // 1. Single Block Add/Remove
        {
            using list_type = TArrayList<int, SIZE>;
            using iterator_type = typename list_type::iterator;

            list_type l;
            TArray<iterator_type> iterators;
            for (SizeT i = 0; i < SIZE; ++i)
            {
                iterator_type it = l.Add(static_cast<int>(i));
                TEST(it != l.end());
                iterators.Add(it);
                TEST(*it == i);
            }
            TEST(l.Capacity() == SIZE);
            for (iterator_type it = l.begin(), end = l.end(); it != end; )
            {
                iterator_type next = it + 1;
                it = l.Remove(it);
                TEST(it == next);
            }
        }
        // 2. Double Block Add/Remove
        {
            using list_type = TArrayList<int, SIZE>;
            using iterator_type = typename list_type::iterator;

            list_type l;
            TArray<iterator_type> iterators;
            for (SizeT i = 0; i < SIZE * 2; ++i)
            {
                iterator_type it = l.Add(static_cast<int>(i));
                TEST(it != l.end());
                iterators.Add(it);
                TEST(*it == i);
            }
            TEST(l.Capacity() == SIZE * 2);
            for (iterator_type it = l.begin(), end = l.end(); it != end; )
            {
                iterator_type next = it + 1;
                it = l.Remove(it);
                TEST(it == next);
            }
        }
        // 3. Three Block Add/Remove
        {
            using list_type = TArrayList<int, SIZE>;
            using iterator_type = typename list_type::iterator;

            list_type l;
            TArray<iterator_type> iterators;
            for (SizeT i = 0; i < SIZE * 3; ++i)
            {
                iterator_type it = l.Add(static_cast<int>(i));
                TEST(it != l.end());
                iterators.Add(it);
                TEST(*it == i);
            }
            TEST(l.Capacity() == SIZE * 3);
            for (iterator_type it = l.begin(), end = l.end(); it != end; )
            {
                iterator_type next = it + 1;
                it = l.Remove(it);
                TEST(it == next);
            }
        }
        // 4. Four Block Add/Remove
        {
            using list_type = TArrayList<int, SIZE>;
            using iterator_type = typename list_type::iterator;

            list_type l;
            TArray<iterator_type> iterators;
            for (SizeT i = 0; i < SIZE * 4; ++i)
            {
                iterator_type it = l.Add(static_cast<int>(i));
                TEST(it != l.end());
                iterators.Add(it);
                TEST(*it == i);
            }
            TEST(l.Capacity() == SIZE * 4);
            for (iterator_type it = l.begin(), end = l.end(); it != end; )
            {
                iterator_type next = it + 1;
                it = l.Remove(it);
                TEST(it == next);
            }
        }
        // 5. Multiblock Add/Remove
        {
            using list_type = TArrayList<int, SIZE>;
            using iterator_type = typename list_type::iterator;

            list_type l;
            TArray<iterator_type> iterators;
            for (SizeT i = 0; i < SIZE * 5; ++i)
            {
                iterator_type it = l.Add(static_cast<int>(i));
                TEST(it != l.end());
                iterators.Add(it);
                TEST(*it == i);
            }
            TEST(l.Capacity() == SIZE * 5);
            for (iterator_type it = l.begin(), end = l.end(); it != end; )
            {
                iterator_type next = it + 1;
                it = l.Remove(it);
                TEST(it == next);
            }
        }

        // Remove End Block
        {
            using list_type = TArrayList<int, SIZE>;
            using iterator_type = typename list_type::iterator;

            list_type l;
            for (SizeT i = 0; i < SIZE; ++i)
            {
                l.Add(static_cast<int>(i));
            }

            TArray<iterator_type> iterators;
            for (SizeT i = 0; i < SIZE; ++i)
            {
                iterators.Add(l.Add(static_cast<int>(i)));
            }

            for (SizeT i = 0; i < iterators.Size(); ++i)
            {
                iterator_type it = iterators[i];
                it = l.Remove(it);
            }
            TEST(l.Size() == SIZE);
            TEST(l.Capacity() == SIZE);
        }

        // Remove Mid Block
        {
            using list_type = TArrayList<int, SIZE>;
            using iterator_type = typename list_type::iterator;

            list_type l;
            for (SizeT i = 0; i < SIZE; ++i)
            {
                l.Add(static_cast<int>(i));
            }

            TArray<iterator_type> iterators;
            for (SizeT i = 0; i < SIZE; ++i)
            {
                iterators.Add(l.Add(static_cast<int>(i)));
            }

            for (SizeT i = 0; i < SIZE; ++i)
            {
                l.Add(static_cast<int>(i));
            }

            for (SizeT i = 0; i < iterators.Size(); ++i)
            {
                iterator_type it = iterators[i];
                it = l.Remove(it);
                TEST(it != l.end());
            }
            TEST(l.Size() == SIZE * 2);
            TEST(l.Capacity() == SIZE * 2);
        }

        // Remove Mid Mid Block
        {
            using list_type = TArrayList<int, SIZE>;
            using iterator_type = typename list_type::iterator;

            list_type l;
            for (SizeT i = 0; i < SIZE * 2; ++i)
            {
                l.Add(static_cast<int>(i));
            }

            TArray<iterator_type> iterators;
            for (SizeT i = 0; i < SIZE; ++i)
            {
                iterators.Add(l.Add(static_cast<int>(i)));
            }

            for (SizeT i = 0; i < SIZE * 2; ++i)
            {
                l.Add(static_cast<int>(i));
            }

            for (SizeT i = 0; i < iterators.Size(); ++i)
            {
                iterator_type it = iterators[i];
                it = l.Remove(it);
                TEST(it != l.end());
            }

            TEST(l.Size() == SIZE * 4);
            TEST(l.Capacity() == SIZE * 4);
        }
        TEST(used == LFGetBytesAllocated());
        // Remove end does nothing:
    }

    static void ArrayListInitializerTest()
    {
        const SizeT SIZE = 5;
        // Remove Mid Mid Block
        {
            using list_type = TArrayList<IntPtr, SIZE>;
            using iterator_type = typename list_type::iterator;
            TArray<IntPtr> safe;

            list_type l;
            for (SizeT i = 0; i < SIZE * 2; ++i)
            {
                safe.Add(MakePtr(static_cast<int>(i)));
                l.Add(safe.GetLast());
            }

            TArray<iterator_type> iterators;
            for (SizeT i = 0; i < SIZE; ++i)
            {
                safe.Add(MakePtr(static_cast<int>(i)));
                iterators.Add(l.Add(safe.GetLast()));
            }

            for (SizeT i = 0; i < SIZE * 2; ++i)
            {
                safe.Add(MakePtr(static_cast<int>(i)));
                l.Add(safe.GetLast());
            }

            for (SizeT i = 0; i < safe.Size(); ++i)
            {
                TEST(safe[i].GetStrongRefs() == 2);
                TEST(safe[i].GetWeakRefs() == 0);
            }

            for (SizeT i = 0; i < iterators.Size(); ++i)
            {
                iterator_type it = iterators[i];
                it = l.Remove(it);
                TEST(it != l.end());
            }

            TArray<IntPtr>::iterator testIt = safe.begin();
            for (SizeT i = 0; i < SIZE * 2; ++i)
            {
                TEST(testIt->GetStrongRefs() == 2);
                TEST(testIt->GetWeakRefs() == 0);
                ++testIt;
            }

            for (SizeT i = 0; i < SIZE; ++i)
            {
                TEST(testIt->GetStrongRefs() == 1);
                TEST(testIt->GetWeakRefs() == 0);
                ++testIt;
            }

            for (SizeT i = 0; i < SIZE * 2; ++i)
            {
                TEST(testIt->GetStrongRefs() == 2);
                TEST(testIt->GetWeakRefs() == 0);
                ++testIt;
            }

            TEST(l.Size() == SIZE * 4);
            TEST(l.Capacity() == SIZE * 4);

            l.Clear();

            for (SizeT i = 0; i < safe.Size(); ++i)
            {
                TEST(safe[i].GetStrongRefs() == 1);
                TEST(safe[i].GetWeakRefs() == 0);
            }
        }
    }

    template<typename T>
    static void TestArrayListAddRemove()
    {
        T a;
        SizeT used = LFGetBytesAllocated();

        TEST(a.Empty());
        TEST(a.Size() == 0);
        TEST(a.Capacity() == 0);
        a.Add(MakePtr(5));
        TEST(a.Size() == 1);
        TEST(a.Capacity() >= 1);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 5);
        CheckReference(a);
        a.Add(MakePtr(7));
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 5, 7);
        CheckReference(a);
        a.Add(MakePtr(3));
        TEST(a.Size() == 3);
        TEST(a.Capacity() >= 3);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 5, 7, 3);
        CheckReference(a);
        // Remove First:
        a.Remove(a.begin());
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 7, 3);
        CheckReference(a);
        a.Add(MakePtr(5));
        TEST(a.Size() == 3);
        TEST(a.Capacity() >= 3);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 5, 7, 3);
        CheckReference(a);
        // Remove Middle:
        a.Remove(a.begin() + 1);
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 5, 3);
        CheckReference(a);
        a.Add(MakePtr(3));
        TEST(a.Size() == 3);
        TEST(a.Capacity() >= 3);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 5, 3, 3);
        CheckReference(a);
        // Remove Last:
        a.Remove(a.begin() + 2);
        TEST(a.Size() == 2);
        TEST(a.Capacity() >= 2);
        TEST(!a.Empty());
        TEST_SEQUENCE(int, a, 5, 3);
        CheckReference(a);
        a.Clear();

        TEST(used == LFGetBytesAllocated());
    }

    REGISTER_TEST(ArrayListTest)
    {
        ArrayListUtilTest();
        ArrayListIteratorTest<TArrayList<int, 10>, TArrayList<int, 10>::iterator>();
        ArrayListIteratorTest<TArrayList<int, 10>, TArrayList<int, 10>::const_iterator>();

        ArrayListSubTest(10);
        ArrayListAddRemove<5>();
        ArrayListAddRemove<10>();
        ArrayListAddRemove<20>();
        ArrayListInitializerTest();

        TestArrayListAddRemove<TArrayList<IntPtr, 10>>();

        TestArrayConstructors<TArrayList<IntPtr, 10>>();

        TestSorting<TArrayList<int, 10>>();
        TestArrayEquality<TArrayList<int, 10>>();
    }
}
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
#include "Core/Test/Test.h"
#include "Core/String/WString.h"
#include "Core/Test/Test.h"

#include <utility>

namespace lf {

    // There are 3 types of strings.

    // Local memory strings. They contain less than 31 characters
    // Heap memory strings. They contain any amount of characters but the memory is allocated on the heap.
    // Copy On Write strings. They contain any amount of characters except the memory is not owned by the string. Therefore it will not be freed!

    static const Char16* LF_WORD_LOCAL = L"Yj34-RwDxa-Ju78";
    static const Char16* LF_WORD_MAX_LOCAL = L"Jx22-Qz17F-LLC0x34-xO3746QR-86";
    static const Char16* LF_WORD_MIN_HEAP = L"IO30-ER45U-BeEMx34-luluZw44-93x";
    static const Char16* LF_WORD_HEAP_LARGE = L"9797d422-c35f-11e7-abc4-cec278b6b50a-zZ2Arg42Lio";
    static const Char16* LF_WORD_HEAP_LARGE_SUB15 = L"9797d422-c35f-1";
    static const SizeT LOCAL_STRING_CAPACITY = LF_STRING_DEFAULT_STORAGE - 2;

    static bool IsLocal(const WString& str)
    {
        return !str.CopyOnWrite() && !str.UseHeap();
    }
    static bool IsHeap(const WString& str)
    {
        return !str.CopyOnWrite() && str.UseHeap();
    }
    static bool IsCopyOnWrite(const WString& str)
    {
        return str.CopyOnWrite() && !str.UseHeap();
    }

    static void TestConstructorAndAssignment()
    {
        // Make an empty string.
        {
            WString empty;
            TEST(IsLocal(empty));
            TEST(empty.Size() == 0);
            TEST(empty.Capacity() == LOCAL_STRING_CAPACITY);
        }
        // Make a local string
        {
            WString local(LF_WORD_MAX_LOCAL);
            TEST(IsLocal(local));
            TEST(local.Size() == 30);
            TEST(local.Capacity() == LOCAL_STRING_CAPACITY);
        }
        // Make a heap string.
        {
            WString heap(LF_WORD_MIN_HEAP);
            TEST(IsHeap(heap));
            TEST(heap.Size() == 31);
            TEST(heap.Capacity() >= 31);
        }
        // Make a copy on write string.
        {
            WString copyOnWrite(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);
            TEST(IsCopyOnWrite(copyOnWrite));

            const SizeT size = copyOnWrite.Size();
            TEST(copyOnWrite.Size() == 48);
            TEST(copyOnWrite.Capacity() >= 48);
        }

        // Copy Constructors:
        {
            WString empty;
            WString local(LF_WORD_MAX_LOCAL);
            WString heap(LF_WORD_MIN_HEAP);
            WString copyOnWrite(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);

            {
                WString temp(empty);
                TEST(IsLocal(temp));
                TEST(temp.Size() == 0);
                TEST(temp.Capacity() == LOCAL_STRING_CAPACITY);
            }

            {
                WString temp(local);
                TEST(IsLocal(temp));
                TEST(temp.Size() == 30);
                TEST(temp.Capacity() == LOCAL_STRING_CAPACITY);
            }

            {
                WString temp(heap);
                TEST(IsHeap(temp));
                TEST(temp.Size() == 31);
                TEST(temp.Capacity() >= 31);
            }

            {
                WString temp(copyOnWrite);
                TEST(IsCopyOnWrite(temp));
                TEST(temp.Size() == 48);
                TEST(temp.Capacity() >= 48);
            }
        }


        // Assignment from string:
        {
            WString empty;
            WString local(LF_WORD_MAX_LOCAL);
            WString heap(LF_WORD_MIN_HEAP);
            WString copyOnWrite(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);

            WString temp;
            temp = empty;
            TEST(IsLocal(temp));
            TEST(temp.Size() == 0);
            TEST(temp.Capacity() == LOCAL_STRING_CAPACITY);

            temp = local;
            TEST(IsLocal(temp));
            TEST(temp.Size() == 30);
            TEST(temp.Capacity() == LOCAL_STRING_CAPACITY);

            temp = heap;
            TEST(IsHeap(temp));
            TEST(temp.Size() == 31);
            TEST(temp.Capacity() >= 31);

            temp = copyOnWrite;
            TEST(IsCopyOnWrite(temp));
            TEST(temp.Size() == 48);
            TEST(temp.Capacity() >= 48);
        }
        // Assignment from cstring:
        {
            WString temp;
            temp = L"";
            TEST(IsLocal(temp));
            TEST(temp.Size() == 0);
            TEST(temp.Capacity() == LOCAL_STRING_CAPACITY);

            temp = LF_WORD_MAX_LOCAL;
            TEST(IsLocal(temp));
            TEST(temp.Size() == 30);
            TEST(temp.Capacity() == LOCAL_STRING_CAPACITY);

            temp = LF_WORD_MIN_HEAP;
            TEST(IsHeap(temp));
            TEST(temp.Size() == 31);
            TEST(temp.Capacity() >= 31);

            // There is no explicit assign for copy on write! Use String(word,COPY_ON_WRITE)
        }


        WString clear(LF_WORD_HEAP_LARGE);
        TEST(IsHeap(clear));
        clear.Clear();
        TEST(!IsHeap(clear));
    }

    static void TestResize()
    {
        WString local(LF_WORD_LOCAL);
        local.Resize(29, 'a');
        TEST(IsLocal(local));
        TEST(local.Size() == 29);
        local.Resize(15, 'b');
        TEST(IsLocal(local));
        TEST(local.Size() == 15);
        local.Resize(17, 'c');
        TEST(IsLocal(local));
        TEST(local.Size() == 17);


        WString CORGrow(LF_WORD_LOCAL, COPY_ON_WRITE);
        CORGrow.Resize(29, 'a');
        TEST(IsLocal(CORGrow));
        TEST(CORGrow.Size() == 29);

        WString CORShrink(LF_WORD_LOCAL, COPY_ON_WRITE);
        CORShrink.Resize(10, 'a');
        TEST(IsLocal(CORShrink));
        TEST(CORShrink.Size() == 10);

        // Empty to Heap:
        WString emptyToHeap;
        emptyToHeap.Resize(33);
        TEST(IsHeap(emptyToHeap));
        TEST(emptyToHeap.Size() == 33);


    }

    static void TestReserve()
    {
        WString local(LF_WORD_LOCAL);
        local.Reserve(29);
        SizeT capacity = local.Capacity();
        TEST(IsLocal(local));
        TEST(local.Size() == 15);
        TEST(local.Capacity() >= 29);
        TEST(local == LF_WORD_LOCAL);

        local.Reserve(15);
        TEST(IsLocal(local));
        TEST(local.Size() == 15);
        TEST(local.Capacity() == capacity);
        TEST(local == LF_WORD_LOCAL);

        local.Reserve(17);
        TEST(IsLocal(local));
        TEST(local.Size() == 15);
        TEST(local.Capacity() == capacity);
        TEST(local == LF_WORD_LOCAL);

        WString heap(LF_WORD_LOCAL);
        heap.Reserve(30);
        capacity = heap.Capacity();;
        TEST(IsHeap(heap));
        TEST(heap.Size() == 15);
        TEST(heap.Capacity() >= 30);
        TEST(heap == LF_WORD_LOCAL);

        heap.Reserve(15);
        TEST(IsHeap(heap));
        TEST(heap.Size() == 15);
        TEST(heap.Capacity() == capacity);
        TEST(heap == LF_WORD_LOCAL);

        heap.Reserve(350);
        TEST(IsHeap(heap));
        TEST(heap.Size() == 15);
        TEST(heap.Capacity() >= 350);
        TEST(heap == LF_WORD_LOCAL);

        WString cor(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);
        cor.Reserve(15);
        TEST(IsLocal(cor));
        TEST(cor.Size() == 15);
        TEST(cor.Capacity() >= 15);
        TEST(cor == LF_WORD_HEAP_LARGE_SUB15);
    }

    static void TestMove()
    {
        WString a(LF_WORD_LOCAL, COPY_ON_WRITE);
        WString b(std::move(a));
        TEST(a.Empty());
        TEST(IsLocal(a));
        TEST(IsCopyOnWrite(b));
        TEST(b.Size() == 15);

        a = std::move(b);
        TEST(b.Empty());
        TEST(IsLocal(b));
        TEST(IsCopyOnWrite(a));
        TEST(a.Size() == 15);

        WString c(LF_WORD_HEAP_LARGE);
        WString d(std::move(c));
        TEST(c.Empty());
        TEST(IsLocal(c));
        TEST(IsHeap(d));
        TEST(d.Size() == 48);

        c = std::move(d);
        TEST(d.Empty());
        TEST(IsLocal(d));
        TEST(IsHeap(c));
        TEST(c.Size() == 48);
    }

    static void TestAppend()
    {
        // Empty appends are the same as assignment. So if there are any errors here there should've been errors in assignment.
        {
            WString empty;
            empty += 'S';
            TEST(IsLocal(empty));
            TEST(empty.Size() == 1);
        }

        {
            WString local;
            local += LF_WORD_MAX_LOCAL;
            TEST(IsLocal(local));
            TEST(local.Size() == 30);
        }
        {
            WString localString(LF_WORD_MAX_LOCAL);
            WString local;
            local += localString;
            TEST(IsLocal(local));
            TEST(local.Size() == 30);
        }

        {
            WString heap;
            heap += LF_WORD_MIN_HEAP;
            TEST(IsHeap(heap));
            TEST(heap.Size() == 31);
        }

        {
            WString heapString(LF_WORD_MIN_HEAP);
            WString heap;
            heap += heapString;
            TEST(IsHeap(heap));
            TEST(heap.Size() == 31);
        }

        {
            WString copyOnWrite(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);
            WString heap;
            heap += copyOnWrite;
            TEST(IsCopyOnWrite(heap)); // Because we are empty and appending.. We just inherit flags from copyOnWrite
            TEST(heap.Size() == 48);
        }

        // Lets try and cover all the cases with all functions.
        WString str15(LF_WORD_LOCAL, COPY_ON_WRITE);

        WString temp(LF_WORD_LOCAL);
        // Test IsLocal
        {
            temp += 'a';
            TEST(IsLocal(temp));
            TEST(temp.Size() == 16);
        }
        temp.Clear();
        temp = LF_WORD_LOCAL;

        {
            temp += LF_WORD_LOCAL;
            TEST(IsLocal(temp));
            TEST(temp.Size() == 30);
        }
        temp.Clear();
        temp = LF_WORD_LOCAL;

        {
            temp += str15;
            TEST(IsLocal(temp));
            TEST(temp.Size() == 30);
        }
        temp.Clear();
        temp = LF_WORD_LOCAL;

        // Test IsHeap
        {
            temp += LF_WORD_LOCAL;
            temp += 'a';
            TEST(IsHeap(temp));
            TEST(temp.Size() == 31);
        }
        temp.Clear();
        temp = LF_WORD_LOCAL;

        {
            temp += 'a';
            temp += LF_WORD_LOCAL;

            TEST(IsHeap(temp));
            TEST(temp.Size() == 31);
        }
        temp.Clear();
        temp = LF_WORD_LOCAL;

        {
            temp += 'a';
            temp += str15;

            TEST(IsHeap(temp));
            TEST(temp.Size() == 31);
        }
        temp.Clear();
        temp = WString(LF_WORD_LOCAL, COPY_ON_WRITE);

        // Test CopyOnWrite to IsLocal
        {
            temp += 'a';
            TEST(IsLocal(temp));
            TEST(temp.Size() == 16);
        }
        temp.Clear();
        temp = WString(LF_WORD_LOCAL, COPY_ON_WRITE);

        {
            temp += LF_WORD_LOCAL;
            TEST(IsLocal(temp));
            TEST(temp.Size() == 30);
        }
        temp.Clear();
        temp = WString(LF_WORD_LOCAL, COPY_ON_WRITE);

        {
            temp += str15;
            TEST(IsLocal(temp));
            TEST(temp.Size() == 30);
        }
        temp.Clear();
        temp = WString(LF_WORD_LOCAL, COPY_ON_WRITE);

        // Test CopyOnWrite to IsHeap
        {
            temp += LF_WORD_LOCAL;
            temp += 'a';
            TEST(IsHeap(temp));
            TEST(temp.Size() == 31);
        }
        temp.Clear();
        temp = WString(LF_WORD_LOCAL, COPY_ON_WRITE);

        {
            temp += 'a';
            temp += LF_WORD_LOCAL;

            TEST(IsHeap(temp));
            TEST(temp.Size() == 31);
        }
        temp.Clear();
        temp = WString(LF_WORD_LOCAL, COPY_ON_WRITE);

        {
            temp += 'a';
            temp += str15;

            TEST(IsHeap(temp));
            TEST(temp.Size() == 31);
        }
        temp.Clear();
        temp = LF_WORD_LOCAL;

        temp += temp; // Append self should result in x2 size.
        TEST(IsLocal(temp));
        TEST(temp.Size() == 30);
        temp += temp;
        TEST(IsHeap(temp));
        TEST(temp.Size() == 60);

        temp.Clear();
        temp = LF_WORD_LOCAL;

        // Operators just use the Append function so as long as the tests for that passed these will to.
        // Operator+
        WString result;
        result = temp + LF_WORD_LOCAL;
        TEST(IsLocal(result));
        TEST(result.Size() == 30);

        result.Clear();
        result = LF_WORD_LOCAL + temp;
        TEST(IsLocal(result));
        TEST(result.Size() == 30);

        result.Clear();
        result = temp + str15;
        TEST(IsLocal(result));
        TEST(result.Size() == 30);
    }

    static void TestEquality()
    {
        const Char16* mismatch15 = L"yJ34-rWdXA-jU78";
        WString temp;
        WString mismatch;
        for (SizeT i = 0; i < 15; ++i)
        {
            temp += LF_WORD_LOCAL;
            mismatch += mismatch15;
        }

        WString a;
        WString b;
        a = temp;
        b = temp;

        TEST(IsHeap(a));
        TEST(IsHeap(b));
        TEST(a.Size() == b.Size());
        TEST(StrEqual(a.CStr(), b.CStr(), a.CStr() + a.Size(), b.CStr() + b.Size()));
        TEST(a == b);
        a = LF_WORD_LOCAL;
        b = LF_WORD_LOCAL;
        TEST(IsHeap(a));
        TEST(IsHeap(b));
        TEST(StrEqual(a.CStr(), b.CStr(), a.CStr() + a.Size(), b.CStr() + b.Size()));
        TEST(a == b);

        a = temp;
        b = mismatch;
        TEST(IsHeap(a));
        TEST(IsHeap(b));
        TEST(a.Size() == b.Size());
        TEST(StrNotEqual(a.CStr(), b.CStr(), a.CStr() + a.Size(), b.CStr() + b.Size()));
        TEST(a != b);

        a = LF_WORD_LOCAL;
        b = mismatch15;
        TEST(IsHeap(a));
        TEST(IsHeap(b));
        TEST(StrNotEqual(a.CStr(), b.CStr(), a.CStr() + a.Size(), b.CStr() + b.Size()));
        TEST(a != b);

        a = temp;
        b = mismatch;
        TEST(a.Size() == b.Size());
        TEST(!StrEqual(a.CStr(), b.CStr(), a.CStr() + a.Size(), b.CStr() + b.Size()));
        TEST(!(a == b));

        a = temp;
        b = temp;
        TEST(a.Size() == b.Size());
        TEST(!StrNotEqual(a.CStr(), b.CStr(), a.CStr() + a.Size(), b.CStr() + b.Size()));
        TEST(!(a != b));


        // simd compare:
        a.Clear();
        b.Clear();
        a = L"banana string exe";
        b = L"banana string exe";
        TEST(IsLocal(a) && IsLocal(b));
        TEST(a == b);
        TEST(!(a != b));
    }

    static void TestInsert()
    {
        // Simple single test:
        {
            WString s(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);
            TEST(s[5] != 'Y');
            s.Insert('Y', 5);
            TEST(s[5] == 'Y');
            TEST(IsHeap(s));
            TEST(s.Size() == 49);
            TEST(s.Capacity() >= 49);
        }

        // Simple multi test
        {
            WString s(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);
            WString sub(L"Yeet", COPY_ON_WRITE);
            for (SizeT i = 5, j = 0; i < s.Size() && j < sub.Size(); ++i, ++j)
            {
                TEST(s[i] != sub[j]);
            }

            s.Insert(sub, 5);
            for (SizeT i = 5, j = 0; i < s.Size() && j < sub.Size(); ++i, ++j)
            {
                TEST(s[i] == sub[j]);
            }
            TEST(IsHeap(s));
            TEST(s.Size() == 52);
            TEST(s.Capacity() >= 52);
        }

        // Other cases will use append? Which is already tested.

    }

    static void TestSubString()
    {
        WString local(L"chicken");
        WString heap(L"heap string is a big large string.");

        WString subLocalA;
        WString subLocalB;
        WString subLocalC;

        local.SubString(1, subLocalA);
        local.SubString(500, subLocalB);
        local.SubString(2, 3, subLocalC);

        TEST(subLocalA == L"hicken");
        TEST(subLocalB == L"");
        TEST(subLocalC == L"ick");

        WString subHeapA;
        WString subHeapB;
        WString subHeapC;

        heap.SubString(1, subHeapA);
        heap.SubString(500, subHeapB);
        heap.SubString(2, 3, subHeapC);

        TEST(subHeapA == L"eap string is a big large string.");
        TEST(subHeapB == L"");
        TEST(subHeapC == L"ap ");
    }

    static void TestReplace()
    {
        WString singleReplace(L"Single");
        WString dualReplace(L"DualDual");
        WString spaceReplace(L"Space replace Space");
        WString noReplace(L"NoReplace");
        WString phrase(L"A string for four where four is five");

        WString scopeReplace(L"Scoped/Name");

        TEST(singleReplace.Replace(L"Single", L"replaced") == 1);
        TEST(singleReplace == (L"replaced"));
        TEST(dualReplace.Replace(L"Dual", L"replaced") == 2);
        TEST(dualReplace == (L"replacedreplaced"));
        TEST(spaceReplace.Replace(L" ", L"_") == 2);
        TEST(spaceReplace == (L"Space_replace_Space"));
        TEST(noReplace.Replace(L"AnySpace", L"replaced") == 0);
        TEST(noReplace == (L"NoReplace"));
        TEST(phrase.Replace(L"four", L"seven") == 2);
        TEST(phrase == (L"A string for seven where seven is five"));
        TEST(phrase.Replace(L"seven", L"") == 2);
        TEST(phrase == (L"A string for  where  is five"));
        TEST(scopeReplace.Replace(L"/", L"::") == 1);
        TEST(scopeReplace == (L"Scoped::Name"));
    }

    static void TestFind()
    {
        // Find Char
        {
            WString sampleA(L"Here for sample is for sample");
            WString sampleB(L"Unique");
            SizeT front = sampleA.Find('H');
            SizeT end = sampleA.FindLast('e');
            TEST(front == 0);
            TEST(end == 28);
            front = sampleB.Find('U');
            end = sampleB.Find('e');
            TEST(front == 0);
            TEST(end == 5);
            front = sampleB.FindLast('U');
            end = sampleB.FindLast('e');
            TEST(front == 0);
            TEST(end == 5);

            char DIR_CHAR = '\\';
            WString sampleC(L"\\REPEAT\\CHAR_FIND");
            front = sampleC.Find(DIR_CHAR);
            end = sampleC.FindLast(DIR_CHAR);
            TEST(front == 0);
            TEST(end == 7);
        }

        // Find String
        {
            WString sampleA(L"Here for sample is for sample");
            WString sampleB(L"Unique");

            SizeT front = sampleA.Find(L"Here");
            SizeT end = sampleA.FindLast(L"sample");

            WString findDefine(L" defined(");
            SizeT i = findDefine.FindLast(L"!defined(");

            TEST(front == 0);
            TEST(end == 23);

            SizeT findLastUnique = sampleB.FindLast(L"Unique");
            TEST(findLastUnique == 0);


            TEST(Invalid(i));
        }
    }

    REGISTER_TEST(WStringTest, "Core.String")
    {
        TestConstructorAndAssignment();
        TestResize();
        TestReserve();
        TestMove();
        TestAppend();
        TestEquality();
        TestInsert();
        TestSubString();
        TestReplace();
        TestFind();
    }

} // namespace lf
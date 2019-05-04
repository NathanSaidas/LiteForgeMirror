#include "StringTest.h"
#include "Core/String/String.h"
#include "Core/String/StringCommon.h"
#include "Core/Test/Test.h"
#include <utility>

namespace lf {

// There are 3 types of strings.

// Local memory strings. They contain less than 31 characters
// Heap memory strings. They contain any amount of characters but the memory is allocated on the heap.
// Copy On Write strings. They contain any amount of characters except the memory is not owned by the string. Therefore it will not be freed!

static const Char8* LF_WORD_LOCAL = "Yj34-RwDxa-Ju78";
static const Char8* LF_WORD_MAX_LOCAL = "Jx22-Qz17F-LLC0x34-xO3746QR-86";
static const Char8* LF_WORD_MIN_HEAP = "IO30-ER45U-BeEMx34-luluZw44-93x";
static const Char8* LF_WORD_HEAP_LARGE = "9797d422-c35f-11e7-abc4-cec278b6b50a-zZ2Arg42Lio";
static const Char8* LF_WORD_HEAP_LARGE_SUB15 = "9797d422-c35f-1";
static const SizeT LOCAL_STRING_CAPACITY = LF_STRING_DEFAULT_STORAGE - 2;

static bool IsLocal(const String& str)
{
    return !str.CopyOnWrite() && !str.UseHeap();
}
static bool IsHeap(const String& str)
{
    return !str.CopyOnWrite() && str.UseHeap();
}
static bool IsCopyOnWrite(const String& str)
{
    return str.CopyOnWrite() && !str.UseHeap();
}

static void TestConstructorAndAssignment()
{
    // Make an empty string.
    {
        String empty;
        TEST(IsLocal(empty));
        TEST(empty.Size() == 0);
        TEST(empty.Capacity() == LOCAL_STRING_CAPACITY);
    }
    // Make a local string
    {
        String local(LF_WORD_MAX_LOCAL);
        TEST(IsLocal(local));
        TEST(local.Size() == 30);
        TEST(local.Capacity() == LOCAL_STRING_CAPACITY);
    }
    // Make a heap string.
    {
        String heap(LF_WORD_MIN_HEAP);
        TEST(IsHeap(heap));
        TEST(heap.Size() == 31);
        TEST(heap.Capacity() >= 31);
    }
    // Make a copy on write string.
    {
        String copyOnWrite(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);
        TEST(IsCopyOnWrite(copyOnWrite));

        const SizeT size = copyOnWrite.Size();
        TEST(copyOnWrite.Size() == 48);
        TEST(copyOnWrite.Capacity() >= 48);
    }

    // Copy Constructors:
    {
        String empty;
        String local(LF_WORD_MAX_LOCAL);
        String heap(LF_WORD_MIN_HEAP);
        String copyOnWrite(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);

        {
            String temp(empty);
            TEST(IsLocal(temp));
            TEST(temp.Size() == 0);
            TEST(temp.Capacity() == LOCAL_STRING_CAPACITY);
        }

        {
            String temp(local);
            TEST(IsLocal(temp));
            TEST(temp.Size() == 30);
            TEST(temp.Capacity() == LOCAL_STRING_CAPACITY);
        }

        {
            String temp(heap);
            TEST(IsHeap(temp));
            TEST(temp.Size() == 31);
            TEST(temp.Capacity() >= 31);
        }

        {
            String temp(copyOnWrite);
            TEST(IsCopyOnWrite(temp));
            TEST(temp.Size() == 48);
            TEST(temp.Capacity() >= 48);
        }
    }


    // Assignment from string:
    {
        String empty;
        String local(LF_WORD_MAX_LOCAL);
        String heap(LF_WORD_MIN_HEAP);
        String copyOnWrite(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);

        String temp;
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
        String temp;
        temp = "";
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


    String clear(LF_WORD_HEAP_LARGE);
    TEST(IsHeap(clear));
    clear.Clear();
    TEST(!IsHeap(clear));
}

static void TestResize()
{
    String local(LF_WORD_LOCAL);
    local.Resize(29, 'a');
    TEST(IsLocal(local));
    TEST(local.Size() == 29);
    local.Resize(15, 'b');
    TEST(IsLocal(local));
    TEST(local.Size() == 15);
    local.Resize(17, 'c');
    TEST(IsLocal(local));
    TEST(local.Size() == 17);


    String CORGrow(LF_WORD_LOCAL, COPY_ON_WRITE);
    CORGrow.Resize(29, 'a');
    TEST(IsLocal(CORGrow));
    TEST(CORGrow.Size() == 29);

    String CORShrink(LF_WORD_LOCAL, COPY_ON_WRITE);
    CORShrink.Resize(10, 'a');
    TEST(IsLocal(CORShrink));
    TEST(CORShrink.Size() == 10);

    // Empty to Heap:
    String emptyToHeap;
    emptyToHeap.Resize(33);
    TEST(IsHeap(emptyToHeap));
    TEST(emptyToHeap.Size() == 33);


}

static void TestReserve()
{
    String local(LF_WORD_LOCAL);
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

    String heap(LF_WORD_LOCAL);
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

    String cor(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);
    cor.Reserve(15);
    TEST(IsLocal(cor));
    TEST(cor.Size() == 15);
    TEST(cor.Capacity() >= 15);
    TEST(cor == LF_WORD_HEAP_LARGE_SUB15);
}

static void TestMove()
{
    String a(LF_WORD_LOCAL, COPY_ON_WRITE);
    String b(std::move(a));
    TEST(a.Empty());
    TEST(IsLocal(a));
    TEST(IsCopyOnWrite(b));
    TEST(b.Size() == 15);

    a = std::move(b);
    TEST(b.Empty());
    TEST(IsLocal(b));
    TEST(IsCopyOnWrite(a));
    TEST(a.Size() == 15);

    String c(LF_WORD_HEAP_LARGE);
    String d(std::move(c));
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
        String empty;
        empty += 'S';
        TEST(IsLocal(empty));
        TEST(empty.Size() == 1);
    }

    {
        String local;
        local += LF_WORD_MAX_LOCAL;
        TEST(IsLocal(local));
        TEST(local.Size() == 30);
    }
    {
        String localString(LF_WORD_MAX_LOCAL);
        String local;
        local += localString;
        TEST(IsLocal(local));
        TEST(local.Size() == 30);
    }

    {
        String heap;
        heap += LF_WORD_MIN_HEAP;
        TEST(IsHeap(heap));
        TEST(heap.Size() == 31);
    }

    {
        String heapString(LF_WORD_MIN_HEAP);
        String heap;
        heap += heapString;
        TEST(IsHeap(heap));
        TEST(heap.Size() == 31);
    }

    {
        String copyOnWrite(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);
        String heap;
        heap += copyOnWrite;
        TEST(IsCopyOnWrite(heap)); // Because we are empty and appending.. We just inherit flags from copyOnWrite
        TEST(heap.Size() == 48);
    }

    // Lets try and cover all the cases with all functions.
    String str15(LF_WORD_LOCAL, COPY_ON_WRITE);

    String temp(LF_WORD_LOCAL);
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
    temp = String(LF_WORD_LOCAL, COPY_ON_WRITE);

    // Test CopyOnWrite to IsLocal
    {
        temp += 'a';
        TEST(IsLocal(temp));
        TEST(temp.Size() == 16);
    }
    temp.Clear();
    temp = String(LF_WORD_LOCAL, COPY_ON_WRITE);

    {
        temp += LF_WORD_LOCAL;
        TEST(IsLocal(temp));
        TEST(temp.Size() == 30);
    }
    temp.Clear();
    temp = String(LF_WORD_LOCAL, COPY_ON_WRITE);

    {
        temp += str15;
        TEST(IsLocal(temp));
        TEST(temp.Size() == 30);
    }
    temp.Clear();
    temp = String(LF_WORD_LOCAL, COPY_ON_WRITE);

    // Test CopyOnWrite to IsHeap
    {
        temp += LF_WORD_LOCAL;
        temp += 'a';
        TEST(IsHeap(temp));
        TEST(temp.Size() == 31);
    }
    temp.Clear();
    temp = String(LF_WORD_LOCAL, COPY_ON_WRITE);

    {
        temp += 'a';
        temp += LF_WORD_LOCAL;

        TEST(IsHeap(temp));
        TEST(temp.Size() == 31);
    }
    temp.Clear();
    temp = String(LF_WORD_LOCAL, COPY_ON_WRITE);

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
    String result;
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
    const char* mismatch15 = "yJ34-rWdXA-jU78";
    String temp;
    String mismatch;
    for (SizeT i = 0; i < 15; ++i)
    {
        temp += LF_WORD_LOCAL;
        mismatch += mismatch15;
    }

    String a;
    String b;
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
    a = "banana string exe";
    b = "banana string exe";
    TEST(IsLocal(a) && IsLocal(b));
    TEST(a == b);
    TEST(!(a != b));
}

static void TestInsert()
{
    // Simple single test:
    {
        String s(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);
        TEST(s[5] != 'Y');
        s.Insert('Y', 5);
        TEST(s[5] == 'Y');
        TEST(IsHeap(s));
        TEST(s.Size() == 49);
        TEST(s.Capacity() >= 49);
    }

    // Simple multi test
    {
        String s(LF_WORD_HEAP_LARGE, COPY_ON_WRITE);
        String sub("Yeet", COPY_ON_WRITE);
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
    String local("chicken");
    String heap("heap string is a big large string.");

    String subLocalA;
    String subLocalB;
    String subLocalC;

    local.SubString(1, subLocalA);
    local.SubString(500, subLocalB);
    local.SubString(2, 3, subLocalC);

    TEST(subLocalA == "hicken");
    TEST(subLocalB == "");
    TEST(subLocalC == "ick");

    String subHeapA;
    String subHeapB;
    String subHeapC;

    heap.SubString(1, subHeapA);
    heap.SubString(500, subHeapB);
    heap.SubString(2, 3, subHeapC);

    TEST(subHeapA == "eap string is a big large string.");
    TEST(subHeapB == "");
    TEST(subHeapC == "ap ");
}

static void TestReplace()
{
    String singleReplace("Single");
    String dualReplace("DualDual");
    String spaceReplace("Space replace Space");
    String noReplace("NoReplace");
    String phrase("A string for four where four is five");

    String scopeReplace("Scoped/Name");

    TEST(singleReplace.Replace("Single", "replaced") == 1);
    TEST(singleReplace == ("replaced"));
    TEST(dualReplace.Replace("Dual", "replaced") == 2);
    TEST(dualReplace == ("replacedreplaced"));
    TEST(spaceReplace.Replace(" ", "_") == 2);
    TEST(spaceReplace == ("Space_replace_Space"));
    TEST(noReplace.Replace("AnySpace", "replaced") == 0);
    TEST(noReplace == ("NoReplace"));
    TEST(phrase.Replace("four", "seven") == 2);
    TEST(phrase == ("A string for seven where seven is five"));
    TEST(phrase.Replace("seven", "") == 2);
    TEST(phrase == ("A string for  where  is five"));
    TEST(scopeReplace.Replace("/", "::") == 1);
    TEST(scopeReplace == ("Scoped::Name"));
}

static void TestFind()
{
    // Find Char
    {
        String sampleA("Here for sample is for sample");
        String sampleB("Unique");
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
        String sampleC("\\REPEAT\\CHAR_FIND");
        front = sampleC.Find(DIR_CHAR);
        end = sampleC.FindLast(DIR_CHAR);
        TEST(front == 0);
        TEST(end == 7);
    }

    // Find String
    {
        String sampleA("Here for sample is for sample");
        String sampleB("Unique");

        SizeT front = sampleA.Find("Here");
        SizeT end = sampleA.FindLast("sample");

        String findDefine(" defined(");
        SizeT i = findDefine.FindLast("!defined(");

        TEST(front == 0);
        TEST(end == 23);

        SizeT findLastUnique = sampleB.FindLast("Unique");
        TEST(findLastUnique == 0);


        TEST(Invalid(i));


    }
}

REGISTER_TEST(String_Regression_StrStripWhitespace)
{
    // This is a bug where the 'badString' is resized to use the heap but is short enough 
    // for a copy to be local.
    //
    // StrStripWhitespace then corrupts the returned string because it thinks its heap

    String badString = "really long string that must use heap";
    badString.Resize(0);
    badString.Append("    ParentUID=4294967295");
    String result = StrStripWhitespace(badString, true);
    TEST_CRITICAL(result == "ParentUID=4294967295");
}

REGISTER_TEST(StringTestCommon)
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

REGISTER_TEST(StringTest)
{
    auto config = TestFramework::GetConfig();
    TestFramework::ExecuteTest("StringTestCommon", config);
    TestFramework::ExecuteTest("String_Regression_StrStripWhitespace", config);
    TestFramework::TestReset();
}

} // namespace lf
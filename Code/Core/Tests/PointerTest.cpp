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
#include "PointerTest.h"
#include "Core/Memory/SmartPointer.h"

namespace lf {

struct TestPointerStruct
{
    static UInt32 sCtor;
    static UInt32 sDtor;

    TestPointerStruct()
    {
        ++sCtor;
        x = 7;
        y = 344;
    }
    ~TestPointerStruct()
    {
        ++sDtor;
    }


    UInt32 x;
    UInt32 y;
};
UInt32 TestPointerStruct::sCtor = 0;
UInt32 TestPointerStruct::sDtor = 0;

static void TestPointerConstructor()
{
    using test_ptr = TStrongPointer<TestPointerStruct>;
    using test_wptr = TWeakPointer<TestPointerStruct>;
    UInt32 ctor = test_ptr::ValueType::sCtor;
    UInt32 dtor = test_ptr::ValueType::sDtor;


    {
        test_ptr ptr;
        TEST(ptr == NULL_PTR);
        TEST(!ptr);
    }

    {
        test_ptr ptr(NULL_PTR);
        TEST(ptr == NULL_PTR);
        TEST(!ptr);
    }

    {
        test_wptr wptr;
        TEST(wptr == NULL_PTR);
        TEST(!wptr);
    }
    // Strong Create
    // Strong Copy(strong)
    {
        void* dummyMemory = LFAlloc(sizeof(test_ptr::ValueType), alignof(test_ptr::ValueType));
        test_ptr::Pointer dummyPointer = new(dummyMemory)TestPointerStruct();
        ++ctor;
        TEST_CRITICAL(dummyMemory == dummyPointer);
        TEST(test_ptr::ValueType::sCtor == ctor);
        test_ptr ptr(dummyPointer);
        TEST(test_ptr::ValueType::sCtor == ctor);
        TEST(ptr != NULL_PTR);
        TEST(!!ptr);
        TEST(ptr.GetStrongRefs() == 1);
        TEST(ptr.GetWeakRefs() == 0);

        test_ptr copy(ptr);
        TEST(ptr != NULL_PTR);
        TEST(!!ptr);
        TEST(ptr.GetStrongRefs() == 2);
        TEST(ptr.GetWeakRefs() == 0);
        TEST(copy != NULL_PTR);
        TEST(!!copy);
        TEST(copy.GetStrongRefs() == 2);
        TEST(copy.GetWeakRefs() == 0);
        TEST(copy == ptr);
        ++dtor;
    }
    TEST(test_ptr::ValueType::sCtor == ctor);
    TEST(test_ptr::ValueType::sDtor == dtor);

    // Strong Create
    // Strong Move
    {
        void* dummyMemory = LFAlloc(sizeof(test_ptr::ValueType), alignof(test_ptr::ValueType));
        test_ptr::Pointer dummyPointer = new(dummyMemory)TestPointerStruct();
        ++ctor;
        TEST_CRITICAL(dummyMemory == dummyPointer);
        TEST(test_ptr::ValueType::sCtor == ctor);
        test_ptr ptr(dummyPointer);
        TEST(test_ptr::ValueType::sCtor == ctor);
        TEST(ptr != NULL_PTR);
        TEST(!!ptr);
        TEST(ptr.GetStrongRefs() == 1);
        TEST(ptr.GetWeakRefs() == 0);

        {
            test_ptr movedPtr(std::move(ptr));
            TEST(ptr == NULL_PTR);
            TEST(!ptr);

            TEST(movedPtr != NULL_PTR);
            TEST(!!movedPtr);
            TEST(movedPtr.GetStrongRefs() == 1);
            TEST(movedPtr.GetWeakRefs() == 0);
            ++dtor;
        }
    }
    TEST(test_ptr::ValueType::sCtor == ctor);
    TEST(test_ptr::ValueType::sDtor == dtor);


    // Strong Create
    // Weak Copy(strong)
    // Strong Copy(weak)
    // Weak Copy(weak)
    // Weak Move(weak)
    {
        void* dummyMemory = LFAlloc(sizeof(test_ptr::ValueType), alignof(test_ptr::ValueType));
        test_ptr::Pointer dummyPointer = new(dummyMemory)TestPointerStruct();
        ++ctor;
        TEST_CRITICAL(dummyMemory == dummyPointer);
        TEST(test_ptr::ValueType::sCtor == ctor);
        test_ptr ptr(dummyPointer);
        TEST(test_ptr::ValueType::sCtor == ctor);
        TEST(ptr != NULL_PTR);
        TEST(!!ptr);
        TEST(ptr.GetStrongRefs() == 1);
        TEST(ptr.GetWeakRefs() == 0);

        test_wptr wptr(ptr);
        TEST(ptr.GetStrongRefs() == 1);
        TEST(ptr.GetWeakRefs() == 1);
        TEST(wptr != NULL_PTR);
        TEST(!!wptr);
        TEST(wptr.GetStrongRefs() == 1);
        TEST(wptr.GetWeakRefs() == 1);

        test_ptr ptrCopy(wptr);
        TEST(ptr.GetStrongRefs() == 2);
        TEST(ptr.GetWeakRefs() == 1);
        TEST(wptr.GetStrongRefs() == 2);
        TEST(wptr.GetWeakRefs() == 1);
        TEST(ptrCopy != NULL_PTR);
        TEST(!!ptrCopy);
        TEST(ptrCopy.GetStrongRefs() == 2);
        TEST(ptrCopy.GetWeakRefs() == 1);

        test_wptr wptrCopy(wptr);
        TEST(wptrCopy == wptr);
        TEST(wptrCopy.GetWeakRefs() == 2);
        TEST(wptrCopy.GetStrongRefs() == 2);
        TEST(wptr.GetWeakRefs() == 2);
        TEST(ptr.GetWeakRefs() == 2);

        test_wptr wptrMove(std::move(wptr));
        TEST(wptrMove == wptrCopy);
        TEST(wptrMove.GetWeakRefs() == 2);
        TEST(wptrMove.GetStrongRefs() == 2);
        TEST(wptrCopy.GetWeakRefs() == 2);
        TEST(ptr.GetWeakRefs() == 2);

        ++dtor;
    }
    TEST(test_ptr::ValueType::sCtor == ctor);
    TEST(test_ptr::ValueType::sDtor == dtor);
}

static void TestPointerEquality()
{
    using test_ptr = TStrongPointer<TestPointerStruct>;
    using test_wptr = TWeakPointer<TestPointerStruct>;

    test_ptr a = test_ptr(LFNew<test_ptr::ValueType>());
    test_ptr b = test_ptr(LFNew<test_ptr::ValueType>());
    test_ptr c = NULL_PTR;

    test_wptr wa(a);
    test_wptr wb(b);
    test_wptr wc(c);

    TEST(a == a);
    TEST(a == wa);
    TEST(wa == a);
    TEST(a != NULL_PTR);
    TEST(wa != NULL_PTR);

    TEST(b == b);
    TEST(b == wb);
    TEST(wb == b);
    TEST(b != NULL_PTR);
    TEST(wb != NULL_PTR);

    TEST(c == c);
    TEST(c == wc);
    TEST(wc == c);
    TEST(c == NULL_PTR);
    TEST(wc == NULL_PTR);

    TEST(a != b);
    TEST(a != wb);
    TEST(wa != b);
    TEST(wa != wb);

}

static void TestPointers()
{
    using test_ptr = TStrongPointer<TestPointerStruct>;
    using test_wptr = TWeakPointer<TestPointerStruct>;

    test_ptr a = test_ptr(LFNew<test_ptr::ValueType>());
    test_ptr b = test_ptr(LFNew<test_ptr::ValueType>());
    test_wptr c(a);
    test_ptr d(a);

    TestPointerConstructor();
    TestPointerEquality();

    // A few extra tests:
    a = a;
    TEST(!!a);
    TEST(a != b);
    {
        test_ptr::ValueType& v = *a;
        a->x = 300;
        a->y = 600;
        TEST(v.x == 300 && v.y == 600);
    }
    a.Release();
    TEST(!a);

    TEST(!!d);
    d.Release();
    TEST(!d);
    TEST(!c);

    TEST(!!b);
    a = b;
    TEST(!!a);
    TEST(a.GetStrongRefs() == 2);
}

REGISTER_TEST(PointerTest)
{
    TestPointers();

}

}
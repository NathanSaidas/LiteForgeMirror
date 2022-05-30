#include "Core/Test/Test.h"
#include "Core/Platform/File.h"
#include "Core/Platform/FileSystem.h"
#include "Core/String/SStream.h"
#include "Core/Utility/SmartCallback.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Time.h"

namespace lf 
{
struct CallbackTester
{
    static SizeT sConstructor;
    static SizeT sCopyConstructor;
    static SizeT sMoveConstructor;
    static SizeT sDestructor;
    static SizeT sCopyAssign;
    static SizeT sMoveAssign;

    static void Reset()
    {
        sConstructor = 0;
        sCopyConstructor = 0;
        sMoveConstructor = 0;
        sDestructor = 0;
        sCopyAssign = 0;
        sMoveAssign = 0;
    }

    static void Output(const char* tag)
    {
        gSysLog.Info(LogMessage("Callback Test Results [") << tag << "]");
        gSysLog.Info(LogMessage("  sContructor: ") << sConstructor);
        gSysLog.Info(LogMessage("  sCopyConstructor: ") << sCopyConstructor);
        gSysLog.Info(LogMessage("  sMoveConstructor: ") << sMoveConstructor);
        gSysLog.Info(LogMessage("  sDestructor: ") << sDestructor);
        gSysLog.Info(LogMessage("  sCopyAssign: ") << sCopyAssign);
        gSysLog.Info(LogMessage("  sMoveAssign: ") << sMoveAssign);
    }

    CallbackTester() : mValue(0)
    {
        ++sConstructor;
    }

    explicit CallbackTester(SizeT value) : mValue(value)
    {
        ++sConstructor;
    }

    CallbackTester(const CallbackTester& other) : mValue(other.mValue)
    {
        ++sCopyConstructor;
    }

    CallbackTester(CallbackTester&& other) : mValue(other.mValue)
    {
        other.mValue = 0;
        ++sMoveConstructor;
    }

    ~CallbackTester()
    {
        ++sDestructor;
    }

    CallbackTester& operator=(const CallbackTester& other)
    {
        mValue = other.mValue;
        ++sCopyAssign;
        return *this;
    }

    CallbackTester& operator=(CallbackTester&& other)
    {
        mValue = other.mValue;
        other.mValue = 0;
        ++sMoveAssign;
        return *this;
    }

    void VoidValue(CallbackTester ) {}
    void VoidRef(CallbackTester&) {}
    void VoidCRef(const CallbackTester&) {}

    static void StaticVoidValue(CallbackTester) {}
    static void StaticVoidRef(CallbackTester&) {}
    static void StaticVoidCRef(const CallbackTester&) {}

    SizeT mValue;
};

SizeT CallbackTester::sConstructor = 0;
SizeT CallbackTester::sCopyConstructor = 0;
SizeT CallbackTester::sMoveConstructor = 0;
SizeT CallbackTester::sDestructor = 0;
SizeT CallbackTester::sCopyAssign = 0;
SizeT CallbackTester::sMoveAssign = 0;

REGISTER_TEST(CallbackTest, "Core.Callback")
{
    // Normal Pointer
    CallbackTester::Reset();
    {
        CallbackTester tester;
        auto callback = TCallback<void, CallbackTester>::Make(&tester, &CallbackTester::VoidValue);
        callback.Invoke(tester);
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 1);
    TEST(CallbackTester::sMoveConstructor == 1);
    TEST(CallbackTester::sDestructor == 3);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("VoidValue");
    CallbackTester::Reset();
    {
        CallbackTester tester;
        auto callback = TCallback<void, CallbackTester&>::Make(&tester, &CallbackTester::VoidRef);

        callback.Invoke(tester);
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 0);
    TEST(CallbackTester::sMoveConstructor == 0);
    TEST(CallbackTester::sDestructor == 1);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("VoidRef");
    CallbackTester::Reset();
    {
        CallbackTester tester;
        auto callback = TCallback<void, const CallbackTester&>::Make(&tester, &CallbackTester::VoidCRef);

        callback.Invoke(tester);
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 0);
    TEST(CallbackTester::sMoveConstructor == 0);
    TEST(CallbackTester::sDestructor == 1);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("VoidCRef");
    CallbackTester::Reset();


    // Smart Pointer
    {
        TStrongPointer<CallbackTester> tester(LFNew<CallbackTester>());
        {
            auto callback = TCallback<void, CallbackTester>::Make(TWeakPointer<CallbackTester>(tester), &CallbackTester::VoidValue);
            callback.Invoke(*tester);
            TEST(tester.GetStrongRefs() == 1);
            TEST(tester.GetWeakRefs() == 1);
        }
        TEST(tester.GetStrongRefs() == 1);
        TEST(tester.GetWeakRefs() == 0);
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 1);
    TEST(CallbackTester::sMoveConstructor == 1);
    TEST(CallbackTester::sDestructor == 3);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("SmartPointer:VoidValue");
    CallbackTester::Reset();
    {
        TStrongPointer<CallbackTester> tester(LFNew<CallbackTester>());
        {
            auto callback = TCallback<void, CallbackTester&>::Make(TWeakPointer<CallbackTester>(tester), &CallbackTester::VoidRef);
            callback.Invoke(*tester);
            TEST(tester.GetStrongRefs() == 1);
            TEST(tester.GetWeakRefs() == 1);
        }
        TEST(tester.GetStrongRefs() == 1);
        TEST(tester.GetWeakRefs() == 0);
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 0);
    TEST(CallbackTester::sMoveConstructor == 0);
    TEST(CallbackTester::sDestructor == 1);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("SmartPointer:VoidRef");
    CallbackTester::Reset();
    {
        TStrongPointer<CallbackTester> tester(LFNew<CallbackTester>());
        {
            auto callback = TCallback<void, const CallbackTester&>::Make(TWeakPointer<CallbackTester>(tester), &CallbackTester::VoidCRef);
            callback.Invoke(*tester);
            TEST(tester.GetStrongRefs() == 1);
            TEST(tester.GetWeakRefs() == 1);
        }
        TEST(tester.GetStrongRefs() == 1);
        TEST(tester.GetWeakRefs() == 0);
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 0);
    TEST(CallbackTester::sMoveConstructor == 0);
    TEST(CallbackTester::sDestructor == 1);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("SmartPointer:VoidCRef");
    CallbackTester::Reset();

    // Atomic Smart Pointer
    {
        TAtomicStrongPointer<CallbackTester> tester(LFNew<CallbackTester>());
        {
            auto callback = TCallback<void, CallbackTester>::Make(TAtomicWeakPointer<CallbackTester>(tester), &CallbackTester::VoidValue);
            callback.Invoke(*tester);
            TEST(tester.GetStrongRefs() == 1);
            TEST(tester.GetWeakRefs() == 1);
        }
        TEST(tester.GetStrongRefs() == 1);
        TEST(tester.GetWeakRefs() == 0);
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 1);
    TEST(CallbackTester::sMoveConstructor == 1);
    TEST(CallbackTester::sDestructor == 3);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("SmartPointer:VoidValue");
    CallbackTester::Reset();
    {
        TAtomicStrongPointer<CallbackTester> tester(LFNew<CallbackTester>());
        {
            auto callback = TCallback<void, CallbackTester&>::Make(TAtomicWeakPointer<CallbackTester>(tester), &CallbackTester::VoidRef);
            callback.Invoke(*tester);
            TEST(tester.GetStrongRefs() == 1);
            TEST(tester.GetWeakRefs() == 1);
        }
        TEST(tester.GetStrongRefs() == 1);
        TEST(tester.GetWeakRefs() == 0);
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 0);
    TEST(CallbackTester::sMoveConstructor == 0);
    TEST(CallbackTester::sDestructor == 1);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("SmartPointer:VoidRef");
    CallbackTester::Reset();
    {
        TAtomicStrongPointer<CallbackTester> tester(LFNew<CallbackTester>());
        {
            auto callback = TCallback<void, const CallbackTester&>::Make(TAtomicWeakPointer<CallbackTester>(tester), &CallbackTester::VoidCRef);
            callback.Invoke(*tester);
            TEST(tester.GetStrongRefs() == 1);
            TEST(tester.GetWeakRefs() == 1);
        }
        TEST(tester.GetStrongRefs() == 1);
        TEST(tester.GetWeakRefs() == 0);
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 0);
    TEST(CallbackTester::sMoveConstructor == 0);
    TEST(CallbackTester::sDestructor == 1);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("SmartPointer:VoidCRef");
    CallbackTester::Reset();
    {
        CallbackTester tester;
        {
            auto callback = TCallback<void, CallbackTester>::Make(CallbackTester::StaticVoidValue);
            callback.Invoke(tester);
        }
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 1);
    TEST(CallbackTester::sMoveConstructor == 1);
    TEST(CallbackTester::sDestructor == 3);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("Function:StaticVoidValue");
    CallbackTester::Reset();
    {
        CallbackTester tester;
        {
            auto callback = TCallback<void, CallbackTester&>::Make(CallbackTester::StaticVoidRef);
            callback.Invoke(tester);
        }
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 0);
    TEST(CallbackTester::sMoveConstructor == 0);
    TEST(CallbackTester::sDestructor == 1);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("Function:StaticVoidRef");
    CallbackTester::Reset();
    {
        CallbackTester tester;
        {
            auto callback = TCallback<void, const CallbackTester&>::Make(CallbackTester::StaticVoidCRef);
            callback.Invoke(tester);
        }
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 0);
    TEST(CallbackTester::sMoveConstructor == 0);
    TEST(CallbackTester::sDestructor == 1);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("Function:StaticVoidCRef");
    CallbackTester::Reset();
    {
        TStrongPointer<CallbackTester> tester(LFNew<CallbackTester>());
        {
            auto callback = TCallback<void, CallbackTester>::Make([tester](CallbackTester value) { tester->VoidValue(value); });
            callback.Invoke(*tester);
            TEST(tester.GetStrongRefs() == 2);
            TEST(tester.GetWeakRefs() == 0);
        }
        TEST(tester.GetStrongRefs() == 1);
        TEST(tester.GetWeakRefs() == 0);
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 2);
    TEST(CallbackTester::sMoveConstructor == 1);
    TEST(CallbackTester::sDestructor == 4);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("Lambda:VoidValue");
    CallbackTester::Reset();
    {
        TStrongPointer<CallbackTester> tester(LFNew<CallbackTester>());
        {
            auto callback = TCallback<void, CallbackTester&>::Make([tester](CallbackTester& value) { tester->VoidRef(value); });
            callback.Invoke(*tester);
            TEST(tester.GetStrongRefs() == 2);
            TEST(tester.GetWeakRefs() == 0);
        }
        TEST(tester.GetStrongRefs() == 1);
        TEST(tester.GetWeakRefs() == 0);
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 0);
    TEST(CallbackTester::sMoveConstructor == 0);
    TEST(CallbackTester::sDestructor == 1);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("Lambda:VoidRef");
    CallbackTester::Reset();
    {
        TStrongPointer<CallbackTester> tester(LFNew<CallbackTester>());
        {
            auto callback = TCallback<void, const CallbackTester&>::Make([tester](const CallbackTester& value) { tester->VoidCRef(value); });
            callback.Invoke(*tester);
            TEST(tester.GetStrongRefs() == 2);
            TEST(tester.GetWeakRefs() == 0);
        }
        TEST(tester.GetStrongRefs() == 1);
        TEST(tester.GetWeakRefs() == 0);
    }
    TEST(CallbackTester::sConstructor == 1);
    TEST(CallbackTester::sCopyConstructor == 0);
    TEST(CallbackTester::sMoveConstructor == 0);
    TEST(CallbackTester::sDestructor == 1);
    TEST(CallbackTester::sCopyAssign == 0);
    TEST(CallbackTester::sMoveAssign == 0);
    CallbackTester::Output("Lambda:VoidCRef");
    CallbackTester::Reset();
}

struct CallbackTestFoo
{
    CallbackTestFoo() : mValue{ 0 } {}
    UInt32 mValue[6];

    void Modify(Float32 value)
    {
        mValue[0] = static_cast<UInt32>(value);
    }

    UInt32 GetValue(SizeT index) const { return mValue[index]; }
};
struct CallbackTestBar
{
    CallbackTestBar() : mValue{ 0.0f } {}
    Float32 mValue[6];
    virtual void Modify(Float32 value)
    {
        mValue[0] = value;
    }

    virtual Float32 GetValue(SizeT index) const { return mValue[index]; }
};
struct CallbackTestBaz : public CallbackTestBar
{
    CallbackTestBaz() : CallbackTestBar() {}

    virtual void Modify(Float32 value)
    {
        mValue[0] = -value;
    }

    virtual Float32 GetValue(SizeT index) const { return -mValue[index]; }
};

REGISTER_TEST(Callback_BindingTest, "Core.Callback")
{
    // Rebinding objects is limited, and not often done. It's suggested to just recreate the whole callback as we don't support RTTI at this level.

    // We can define a signature of a callback
    using Signature = TCallback<void, Float32>;

    // And we can bind an object & method pointer to the callback
    CallbackTestBar bar;
    Signature callback(Signature::Make(&bar, &CallbackTestBar::Modify));

    // We cannot re-bind T* to SmartPtr or AtomicSmartPtr
    TStrongPointer<CallbackTestBar> smartBar(LFNew<CallbackTestBar>());
    TEST(!callback.BindObject(smartBar));

    TAtomicStrongPointer<CallbackTestBar> atomicSmartBar(LFNew<CallbackTestBar>());
    TEST(!callback.BindObject(atomicSmartBar));

    // We cannot re-bind to different class type.
    CallbackTestFoo foo;
    TEST(!callback.BindObject(&foo));

    // We cannot re-bind like types.
    CallbackTestBaz baz;
    TEST(!callback.BindObject(&baz));
    
    // But we can re-initialize
    callback = Signature::Make(&baz, callback.GetMethodPtr<CallbackTestBar>());

    // callback = Signature::Make(&baz, &)
}

using VoidCallback = TCallback<void>;
// Value
using ValueArgCallback = TCallback<void, CallbackTester>;
using ValueReturnCallback = TCallback<CallbackTester>;
using ValueReturnArgCallback = TCallback<CallbackTester, CallbackTester>;
// Reference
using RefArgCallback = TCallback<void, CallbackTester&>;
using RefReturnCallback = TCallback<CallbackTester&>;
using RefReturnArgCallback = TCallback<CallbackTester&, CallbackTester&>;
// Const Reference
using CRefArgCallback = TCallback<void, const CallbackTester&>;
using CRefReturnCallback = TCallback<const CallbackTester&>;
using CRefReturnArgCallback = TCallback<const CallbackTester&, const CallbackTester&>;
// Multi Arg
using MultiArgCallback = TCallback<CallbackTester, CallbackTester, CallbackTester&, const CallbackTester&>;

struct CallbackTestFunctions
{
    static CallbackTester sInstance;

    void                    Void() { }
    void                    VoidConst() const { }
    static void             StaticVoid() { }

    void                    ValueArg(CallbackTester) {}
    void                    ValueArgConst(CallbackTester) const {}
    static void             StaticValueArg(CallbackTester) {}

    CallbackTester          ValueReturn() { return CallbackTester(); }
    CallbackTester          ValueReturnConst() const { return CallbackTester(); }
    static CallbackTester   StaticValueReturn() { return CallbackTester(); }

    CallbackTester          ValueReturnArg(CallbackTester) { return CallbackTester(); }
    CallbackTester          ValueReturnArgConst(CallbackTester) const { return CallbackTester(); }
    static CallbackTester   StaticValueReturnArg(CallbackTester) { return CallbackTester(); }

    void                    RefArg(CallbackTester&) {}
    void                    RefArgConst(CallbackTester&) const {}
    static void             StaticRefArg(CallbackTester&) {}

    CallbackTester&         RefReturn() { return sInstance; }
    CallbackTester&         RefReturnConst() const { return sInstance;}
    static CallbackTester&  StaticRefReturn() { return sInstance;}

    CallbackTester&         RefReturnArg(CallbackTester&) { return sInstance;}
    CallbackTester&         RefReturnArgConst(CallbackTester&) const { return sInstance; }
    static CallbackTester&  StaticRefReturnArg(CallbackTester&) { return sInstance; }

    void                    CRefArg(const CallbackTester&) {}
    void                    CRefArgConst(const CallbackTester&) const {}
    static void             StaticCRefArg(const CallbackTester&) {}

    const CallbackTester&   CRefReturn() { return sInstance; }
    const CallbackTester&   CRefReturnConst() const { return sInstance; }
    static const CallbackTester&  StaticCRefReturn() { return sInstance; }

    const CallbackTester&   CRefReturnArg(const CallbackTester&) { return sInstance; }
    const CallbackTester&   CRefReturnArgConst(const CallbackTester&) const { return sInstance; }
    static const CallbackTester& StaticCRefReturnArg(const CallbackTester&) { return sInstance; }

    CallbackTester          Multi(CallbackTester value, CallbackTester& ref, const CallbackTester& cref)
    {
        CallbackTester old = ref;
        ref = cref;
        value.mValue = 9999;
        return old;
    }

    CallbackTester          MultiConst(CallbackTester value, CallbackTester& ref, const CallbackTester& cref) const
    {
        CallbackTester old = ref;
        ref = cref;
        value.mValue = 9999;
        return old;
    }

    static CallbackTester   StaticMulti(CallbackTester value, CallbackTester& ref, const CallbackTester& cref)
    {
        CallbackTester old = ref;
        ref = cref;
        value.mValue = 9999;
        return old;
    }

};
CallbackTester CallbackTestFunctions::sInstance;

REGISTER_TEST(Callback_DefaultConstructor, "Core.Callback")
{
    {
        VoidCallback callback;
        TEST(!callback.IsValid());
    }
    // Value
    {
        ValueArgCallback callback;
        TEST(!callback.IsValid());
    }
    {
        ValueReturnCallback callback;
        TEST(!callback.IsValid());
    }
    {
        ValueReturnArgCallback callback;
        TEST(!callback.IsValid());
    }
    // Reference
    {
        RefArgCallback callback;
        TEST(!callback.IsValid());
    }
    {
        RefReturnCallback callback;
        TEST(!callback.IsValid());
    }
    {
        RefReturnArgCallback callback;
        TEST(!callback.IsValid());
    }
    // Const Reference
    {
        CRefArgCallback callback;
        TEST(!callback.IsValid());
    }
    {
        CRefReturnCallback callback;
        TEST(!callback.IsValid());
    }
    {
        CRefReturnArgCallback callback;
        TEST(!callback.IsValid());
    }
    // Multi Arg
    {
        MultiArgCallback callback;
        TEST(!callback.IsValid());
    }
}

REGISTER_TEST(Callback_Make_Void, "Core.Callback")
{
    // FunctionType
    {
        VoidCallback callback(VoidCallback::Make(CallbackTestFunctions::StaticVoid));
        TEST(callback.IsValid());
        TEST(callback.IsFunction());
    }
    // LambdaType
    {
        VoidCallback callback(VoidCallback::Make([]() { CallbackTestFunctions::StaticVoid(); }));
        TEST(callback.IsValid());
        TEST(callback.IsLambda());
    }
    // MethodType<T*>
    {
        CallbackTestFunctions binder;
        VoidCallback callback(VoidCallback::Make(&binder, &CallbackTestFunctions::Void));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        VoidCallback callback(VoidCallback::Make(binder, &CallbackTestFunctions::Void));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        VoidCallback callback(VoidCallback::Make(binder, &CallbackTestFunctions::Void));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // ConstMethodType<T*>
    {
        CallbackTestFunctions binder;
        VoidCallback callback(VoidCallback::Make(&binder, &CallbackTestFunctions::VoidConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        VoidCallback callback(VoidCallback::Make(binder, &CallbackTestFunctions::VoidConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        VoidCallback callback(VoidCallback::Make(binder, &CallbackTestFunctions::VoidConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
}

REGISTER_TEST(Callback_Make_ValueArg, "Core.Callback")
{
    // FunctionType
    {
        ValueArgCallback callback(ValueArgCallback::Make(CallbackTestFunctions::StaticValueArg));
        TEST(callback.IsValid());
        TEST(callback.IsFunction());
    }
    // LambdaType
    {
        ValueArgCallback callback(ValueArgCallback::Make([](CallbackTester value) { CallbackTestFunctions::StaticValueArg(value); }));
        TEST(callback.IsValid());
        TEST(callback.IsLambda());
    }
    // MethodType<T*>
    {
        CallbackTestFunctions binder;
        ValueArgCallback callback(ValueArgCallback::Make(&binder, &CallbackTestFunctions::ValueArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        ValueArgCallback callback(ValueArgCallback::Make(binder, &CallbackTestFunctions::ValueArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        ValueArgCallback callback(ValueArgCallback::Make(binder, &CallbackTestFunctions::ValueArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // ConstMethodType<T*>
    {
        CallbackTestFunctions binder;
        ValueArgCallback callback(ValueArgCallback::Make(&binder, &CallbackTestFunctions::ValueArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        ValueArgCallback callback(ValueArgCallback::Make(binder, &CallbackTestFunctions::ValueArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        ValueArgCallback callback(ValueArgCallback::Make(binder, &CallbackTestFunctions::ValueArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
}

REGISTER_TEST(Callback_Make_ValueReturn, "Core.Callback")
{
    // FunctionType
    {
        ValueReturnCallback callback(ValueReturnCallback::Make(CallbackTestFunctions::StaticValueReturn));
        TEST(callback.IsValid());
        TEST(callback.IsFunction());
    }
    // LambdaType
    {
        ValueReturnCallback callback(ValueReturnCallback::Make([]() { return CallbackTestFunctions::StaticValueReturn(); }));
        TEST(callback.IsValid());
        TEST(callback.IsLambda());
    }
    // MethodType<T*>
    {
        CallbackTestFunctions binder;
        ValueReturnCallback callback(ValueReturnCallback::Make(&binder, &CallbackTestFunctions::ValueReturn));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        ValueReturnCallback callback(ValueReturnCallback::Make(binder, &CallbackTestFunctions::ValueReturn));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        ValueReturnCallback callback(ValueReturnCallback::Make(binder, &CallbackTestFunctions::ValueReturn));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // ConstMethodType<T*>
    {
        CallbackTestFunctions binder;
        ValueReturnCallback callback(ValueReturnCallback::Make(&binder, &CallbackTestFunctions::ValueReturnConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        ValueReturnCallback callback(ValueReturnCallback::Make(binder, &CallbackTestFunctions::ValueReturnConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        ValueReturnCallback callback(ValueReturnCallback::Make(binder, &CallbackTestFunctions::ValueReturnConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
}

REGISTER_TEST(Callback_Make_ValueReturnArg, "Core.Callback")
{
    // FunctionType
    {
        ValueReturnArgCallback callback(ValueReturnArgCallback::Make(CallbackTestFunctions::StaticValueReturnArg));
        TEST(callback.IsValid());
        TEST(callback.IsFunction());
    }
    // LambdaType
    {
        ValueReturnArgCallback callback(ValueReturnArgCallback::Make([](CallbackTester value) { return CallbackTestFunctions::StaticValueReturnArg(value); }));
        TEST(callback.IsValid());
        TEST(callback.IsLambda());
    }
    // MethodType<T*>
    {
        CallbackTestFunctions binder;
        ValueReturnArgCallback callback(ValueReturnArgCallback::Make(&binder, &CallbackTestFunctions::ValueReturnArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        ValueReturnArgCallback callback(ValueReturnArgCallback::Make(binder, &CallbackTestFunctions::ValueReturnArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        ValueReturnArgCallback callback(ValueReturnArgCallback::Make(binder, &CallbackTestFunctions::ValueReturnArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // ConstMethodType<T*>
    {
        CallbackTestFunctions binder;
        ValueReturnArgCallback callback(ValueReturnArgCallback::Make(&binder, &CallbackTestFunctions::ValueReturnArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        ValueReturnArgCallback callback(ValueReturnArgCallback::Make(binder, &CallbackTestFunctions::ValueReturnArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        ValueReturnArgCallback callback(ValueReturnArgCallback::Make(binder, &CallbackTestFunctions::ValueReturnArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
}

REGISTER_TEST(Callback_Make_RefArg, "Core.Callback")
{
    // FunctionType
    {
        RefArgCallback callback(RefArgCallback::Make(CallbackTestFunctions::StaticRefArg));
        TEST(callback.IsValid());
        TEST(callback.IsFunction());
    }
    // LambdaType
    {
        RefArgCallback callback(RefArgCallback::Make([](CallbackTester& value) { CallbackTestFunctions::StaticRefArg(value); }));
        TEST(callback.IsValid());
        TEST(callback.IsLambda());
    }
    // MethodType<T*>
    {
        CallbackTestFunctions binder;
        RefArgCallback callback(RefArgCallback::Make(&binder, &CallbackTestFunctions::RefArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        RefArgCallback callback(RefArgCallback::Make(binder, &CallbackTestFunctions::RefArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        RefArgCallback callback(RefArgCallback::Make(binder, &CallbackTestFunctions::RefArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // ConstMethodType<T*>
    {
        CallbackTestFunctions binder;
        RefArgCallback callback(RefArgCallback::Make(&binder, &CallbackTestFunctions::RefArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        RefArgCallback callback(RefArgCallback::Make(binder, &CallbackTestFunctions::RefArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        RefArgCallback callback(RefArgCallback::Make(binder, &CallbackTestFunctions::RefArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
}

REGISTER_TEST(Callback_Make_RefReturn, "Core.Callback")
{
    // FunctionType
    {
        RefReturnCallback callback(RefReturnCallback::Make(CallbackTestFunctions::StaticRefReturn));
        TEST(callback.IsValid());
        TEST(callback.IsFunction());
    }
    // LambdaType
    {
        RefReturnCallback callback(RefReturnCallback::Make([]() -> CallbackTester& { return CallbackTestFunctions::StaticRefReturn(); }));
        TEST(callback.IsValid());
        TEST(callback.IsLambda());
    }
    // MethodType<T*>
    {
        CallbackTestFunctions binder;
        RefReturnCallback callback(RefReturnCallback::Make(&binder, &CallbackTestFunctions::RefReturn));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        RefReturnCallback callback(RefReturnCallback::Make(binder, &CallbackTestFunctions::RefReturn));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        RefReturnCallback callback(RefReturnCallback::Make(binder, &CallbackTestFunctions::RefReturn));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // ConstMethodType<T*>
    {
        CallbackTestFunctions binder;
        RefReturnCallback callback(RefReturnCallback::Make(&binder, &CallbackTestFunctions::RefReturnConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        RefReturnCallback callback(RefReturnCallback::Make(binder, &CallbackTestFunctions::RefReturnConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        RefReturnCallback callback(RefReturnCallback::Make(binder, &CallbackTestFunctions::RefReturnConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
}

REGISTER_TEST(Callback_Make_RefReturnArg, "Core.Callback")
{
    // FunctionType
    {
        RefReturnArgCallback callback(RefReturnArgCallback::Make(CallbackTestFunctions::StaticRefReturnArg));
        TEST(callback.IsValid());
        TEST(callback.IsFunction());
    }
    // LambdaType
    {
        RefReturnArgCallback callback(RefReturnArgCallback::Make([](CallbackTester& value) -> CallbackTester& { return CallbackTestFunctions::StaticRefReturnArg(value); }));
        TEST(callback.IsValid());
        TEST(callback.IsLambda());
    }
    // MethodType<T*>
    {
        CallbackTestFunctions binder;
        RefReturnArgCallback callback(RefReturnArgCallback::Make(&binder, &CallbackTestFunctions::RefReturnArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        RefReturnArgCallback callback(RefReturnArgCallback::Make(binder, &CallbackTestFunctions::RefReturnArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        RefReturnArgCallback callback(RefReturnArgCallback::Make(binder, &CallbackTestFunctions::RefReturnArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // ConstMethodType<T*>
    {
        CallbackTestFunctions binder;
        RefReturnArgCallback callback(RefReturnArgCallback::Make(&binder, &CallbackTestFunctions::RefReturnArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        RefReturnArgCallback callback(RefReturnArgCallback::Make(binder, &CallbackTestFunctions::RefReturnArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        RefReturnArgCallback callback(RefReturnArgCallback::Make(binder, &CallbackTestFunctions::RefReturnArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
}

REGISTER_TEST(Callback_Make_CRefArg, "Core.Callback")
{
    // FunctionType
    {
        CRefArgCallback callback(CRefArgCallback::Make(CallbackTestFunctions::StaticCRefArg));
        TEST(callback.IsValid());
        TEST(callback.IsFunction());
    }
    // LambdaType
    {
        CRefArgCallback callback(CRefArgCallback::Make([](const CallbackTester& value) { return CallbackTestFunctions::StaticCRefArg(value); }));
        TEST(callback.IsValid());
        TEST(callback.IsLambda());
    }
    // MethodType<T*>
    {
        CallbackTestFunctions binder;
        CRefArgCallback callback(CRefArgCallback::Make(&binder, &CallbackTestFunctions::CRefArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        CRefArgCallback callback(CRefArgCallback::Make(binder, &CallbackTestFunctions::CRefArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        CRefArgCallback callback(CRefArgCallback::Make(binder, &CallbackTestFunctions::CRefArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // ConstMethodType<T*>
    {
        CallbackTestFunctions binder;
        CRefArgCallback callback(CRefArgCallback::Make(&binder, &CallbackTestFunctions::CRefArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        CRefArgCallback callback(CRefArgCallback::Make(binder, &CallbackTestFunctions::CRefArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        CRefArgCallback callback(CRefArgCallback::Make(binder, &CallbackTestFunctions::CRefArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
}

REGISTER_TEST(Callback_Make_CRefReturn, "Core.Callback")
{
    // FunctionType
    {
        CRefReturnCallback callback(CRefReturnCallback::Make(CallbackTestFunctions::StaticCRefReturn));
        TEST(callback.IsValid());
        TEST(callback.IsFunction());
    }
    // LambdaType
    {
        CRefReturnCallback callback(CRefReturnCallback::Make([]() -> const CallbackTester& { return CallbackTestFunctions::StaticCRefReturn(); }));
        TEST(callback.IsValid());
        TEST(callback.IsLambda());
    }
    // MethodType<T*>
    {
        CallbackTestFunctions binder;
        CRefReturnCallback callback(CRefReturnCallback::Make(&binder, &CallbackTestFunctions::CRefReturn));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        CRefReturnCallback callback(CRefReturnCallback::Make(binder, &CallbackTestFunctions::CRefReturn));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        CRefReturnCallback callback(CRefReturnCallback::Make(binder, &CallbackTestFunctions::CRefReturn));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // ConstMethodType<T*>
    {
        CallbackTestFunctions binder;
        CRefReturnCallback callback(CRefReturnCallback::Make(&binder, &CallbackTestFunctions::CRefReturnConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        CRefReturnCallback callback(CRefReturnCallback::Make(binder, &CallbackTestFunctions::CRefReturnConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        CRefReturnCallback callback(CRefReturnCallback::Make(binder, &CallbackTestFunctions::CRefReturnConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
}

REGISTER_TEST(Callback_Make_CRefReturnArg, "Core.Callback")
{
    // FunctionType
    {
        CRefReturnArgCallback callback(CRefReturnArgCallback::Make(CallbackTestFunctions::StaticCRefReturnArg));
        TEST(callback.IsValid());
        TEST(callback.IsFunction());
    }
    // LambdaType
    {
        CRefReturnArgCallback callback(CRefReturnArgCallback::Make([](const CallbackTester& value) -> const CallbackTester& { return CallbackTestFunctions::StaticCRefReturnArg(value); }));
        TEST(callback.IsValid());
        TEST(callback.IsLambda());
    }
    // MethodType<T*>
    {
        CallbackTestFunctions binder;
        CRefReturnArgCallback callback(CRefReturnArgCallback::Make(&binder, &CallbackTestFunctions::CRefReturnArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        CRefReturnArgCallback callback(CRefReturnArgCallback::Make(binder, &CallbackTestFunctions::CRefReturnArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // MethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        CRefReturnArgCallback callback(CRefReturnArgCallback::Make(binder, &CallbackTestFunctions::CRefReturnArg));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
    }
    // ConstMethodType<T*>
    {
        CallbackTestFunctions binder;
        CRefReturnArgCallback callback(CRefReturnArgCallback::Make(&binder, &CallbackTestFunctions::CRefReturnArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        CRefReturnArgCallback callback(CRefReturnArgCallback::Make(binder, &CallbackTestFunctions::CRefReturnArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
    // ConstMethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        CRefReturnArgCallback callback(CRefReturnArgCallback::Make(binder, &CallbackTestFunctions::CRefReturnArgConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
    }
}

// Test to make sure we can invoke call types.

static void VerifyMultiInvoke(MultiArgCallback& callback)
{
    CallbackTester foo(5);
    CallbackTester bar(10);
    CallbackTester baz(15);
    CallbackTester returned = callback.Invoke(foo, bar, baz);

    TEST(foo.mValue != bar.mValue);
    TEST(foo.mValue != baz.mValue);
    TEST(bar.mValue == baz.mValue);
    TEST(returned.mValue == 10);
}

REGISTER_TEST(Callback_Invoke_Multi, "Core.Callback")
{
    // FunctionType
    {
        MultiArgCallback callback(MultiArgCallback::Make(CallbackTestFunctions::StaticMulti));
        TEST(callback.IsValid());
        TEST(callback.IsFunction());
        VerifyMultiInvoke(callback);
    }
    // LambdaType
    {
        MultiArgCallback callback(MultiArgCallback::Make(
            [](CallbackTester value, CallbackTester& ref, const CallbackTester& cref) -> CallbackTester
            { return CallbackTestFunctions::StaticMulti(value, ref, cref); }));
        TEST(callback.IsValid());
        TEST(callback.IsLambda());
        VerifyMultiInvoke(callback);
    }
    // MethodType<T*>
    {
        CallbackTestFunctions binder;
        MultiArgCallback callback(MultiArgCallback::Make(&binder, &CallbackTestFunctions::Multi));
        TEST(callback.IsValid());
        TEST(callback.IsMethod());
        VerifyMultiInvoke(callback);
    }
    // MethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        {
            MultiArgCallback callback(MultiArgCallback::Make(binder, &CallbackTestFunctions::Multi));
            TEST(callback.IsValid());
            TEST(callback.IsMethod());
            TEST(binder.GetStrongRefs() == 1);
            TEST(binder.GetWeakRefs() == 1);
            VerifyMultiInvoke(callback);
        }
        TEST(binder.GetStrongRefs() == 1);
        TEST(binder.GetWeakRefs() == 0);
    }
    // MethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        {
            MultiArgCallback callback(MultiArgCallback::Make(binder, &CallbackTestFunctions::Multi));
            TEST(callback.IsValid());
            TEST(callback.IsMethod());
            TEST(binder.GetStrongRefs() == 1);
            TEST(binder.GetWeakRefs() == 1);
            VerifyMultiInvoke(callback);
        }
        TEST(binder.GetStrongRefs() == 1);
        TEST(binder.GetWeakRefs() == 0);
    }
    // ConstMethodType<T*>
    {
        CallbackTestFunctions binder;
        MultiArgCallback callback(MultiArgCallback::Make(&binder, &CallbackTestFunctions::MultiConst));
        TEST(callback.IsValid());
        TEST(callback.IsConstMethod());
        VerifyMultiInvoke(callback);
    }
    // ConstMethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        {
            MultiArgCallback callback(MultiArgCallback::Make(binder, &CallbackTestFunctions::MultiConst));
            TEST(callback.IsValid());
            TEST(callback.IsConstMethod());
            TEST(binder.GetStrongRefs() == 1);
            TEST(binder.GetWeakRefs() == 1);
            VerifyMultiInvoke(callback);
        }
        TEST(binder.GetStrongRefs() == 1);
        TEST(binder.GetWeakRefs() == 0);
    }
    // ConstMethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        {
            MultiArgCallback callback(MultiArgCallback::Make(binder, &CallbackTestFunctions::MultiConst));
            TEST(callback.IsValid());
            TEST(callback.IsConstMethod());
            TEST(binder.GetStrongRefs() == 1);
            TEST(binder.GetWeakRefs() == 1);
            VerifyMultiInvoke(callback);
        }
        TEST(binder.GetStrongRefs() == 1);
        TEST(binder.GetWeakRefs() == 0);
    }
}

REGISTER_TEST(Callback_ObserveSmartPtr, "Core.Callback")
{
    // MethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        {
            MultiArgCallback callback(MultiArgCallback::Make(binder, &CallbackTestFunctions::Multi));
            TEST(callback.IsValid());
            binder = NULL_PTR;
            TEST(!callback.IsValid());
        }
    }
    // MethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        {
            MultiArgCallback callback(MultiArgCallback::Make(binder, &CallbackTestFunctions::Multi));
            TEST(callback.IsValid());
            binder = NULL_PTR;
            TEST(!callback.IsValid());
        }
    }
    // ConstMethodType<TWeakPointer<T>>
    {
        TStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        {
            MultiArgCallback callback(MultiArgCallback::Make(binder, &CallbackTestFunctions::MultiConst));
            TEST(callback.IsValid());
            binder = NULL_PTR;
            TEST(!callback.IsValid());
        }
    }
    // ConstMethodType<TAtomicWeakPointer<T>>
    {
        TAtomicStrongPointer<CallbackTestFunctions> binder(LFNew<CallbackTestFunctions>());
        {
            MultiArgCallback callback(MultiArgCallback::Make(binder, &CallbackTestFunctions::MultiConst));
            TEST(callback.IsValid());
            binder = NULL_PTR;
            TEST(!callback.IsValid());
        }
    }
}

DECLARE_HASHED_CALLBACK(CacheWriteResolver, void);

} // namespace lf
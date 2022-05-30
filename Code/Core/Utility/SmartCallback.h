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

#pragma once
#include "Core/Memory/SmartPointer.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Utility/FNVHash.h"
#include <utility>

namespace lf
{

    // ********************************************************************
    // 
    // TCallback< ReturnT, ArgsT ... >
    //      This is the base class for all callback types, its a wrapper around
    //      function/lambda/method/const method pointers, as well supports 
    //      object binding for raw/smart/atomic-smart pointers.
    // 
    //      TCallback is designed to be 'type-safe' but not dynamic, eg doesn't
    //      support object binding for polymorphic types (re-create the callback)
    //
    //      Use TCallback<...>::Make( ... ) to create a callback
    // 
    // THashedCallback< Tag, ReturnT, Args ... >
    //      This is a extension of 'TCallback' that has built-in hashing 
    //      functionality WHEN USED WITH the macros DECLARE_HASHED_CALLBACK/
    //      DECLARE_HASHED_CALLBACK_WITH_SIZE.
    // 
    //      Hashed callbacks allow you to convert between anonymous callback
    //      and back to hashed callback (as long as the signature matches)
    //
    // Signature: The signature of a callback is defined as the arguments
    //      you passed in for ReturnT / ArgsT (ignoring spaces)
    //
    // Anonymous Callback: A representation of a callback in data only
    //      (runtime safe only, do not save to memory!) 
    //
    // Usage w/ hashing:
    //          DECLARE_HASHED_CALLBACK( Name, ReturnT, ArgsT ... )
    //          DECLARE_HASHED_CALLBACK_WITH_SIZE( Name, ReturnT, ArgsT ... )
    // 
    // Usage w/o hashing:
    //          using Name = TCallback<ReturnT, ArgsT...>
    // 
    // ********************************************************************


#define DECLARE_HASHED_CALLBACK(Name, ...)                              \
using Name = THashedCallbackBase<ComputeCallbackHash(#__VA_ARGS__), 64, __VA_ARGS__>;   \

#define DECLARE_HASHED_CALLBACK_WITH_SIZE(Name, Size, ...)              \
using Name = THashedCallbackBase<ComputeCallbackHash(#__VA_ARGS__), Size, __VA_ARGS__>; \

    template<SizeT CBufferSize>
    LF_INLINE constexpr FNV::HashT ComputeCallbackHash(const char(&string)[CBufferSize])
    {
        FNV::HashT hash = FNV::FNV_OFFSET_BASIS;
        for (SizeT i = 0; i < CBufferSize; ++i)
        {
            if (string[i] == ' ' || string[i] == '\t' || string[i] == '\0')
            {
                continue;
            }
            hash = hash * FNV::FNV_PRIME;
            hash = hash ^ string[i];
        }
        return hash;
    }

    enum class CallbackType : SizeT
    {
        CT_FUNCTION,
        CT_LAMBDA,
        CT_METHOD,
        CT_WEAK_PTR_METHOD,
        CT_ATOMIC_WEAK_PTR_METHOD,
        CT_CONST_METHOD,
        CT_WEAK_PTR_CONST_METHOD,
        CT_ATOMIC_WEAK_PTR_CONST_METHOD,
        MAX_VALUE,
        INVALID_ENUM
    };

    template<SizeT CBufferSize>
    struct TAnonymousCallback
    {
        FNV::HashT      mSignatureHash;
        CallbackType    mType;
        ByteT           mData[CBufferSize];
    };

    struct AnonymousCallback
    {
        FNV::HashT   mSignatureHash;
        CallbackType mType;
        ByteT        mData[64];
    };

    struct ArgumentPackVoid
    {};

    template<typename T, typename ... ArgsT>
    struct TArgumentPackBase
    {
        TArgumentPackBase(T value, ArgsT ... args)
            : mValue(value)
            , mNext(std::forward<ArgsT>(args)...)
        {}

        template<typename CallbackT, typename ... ArgsT>
        typename CallbackT::ReturnType Invoke(CallbackT& callback, ArgsT&& ... args)
        {
            return mNext.Invoke(callback, std::forward<T>(mValue), std::forward<ArgsT>(args)...);
        }

        T mValue;
        TArgumentPackBase<ArgsT...> mNext;
    };

    template<>
    struct TArgumentPackBase<ArgumentPackVoid>
    {
        TArgumentPackBase()
        {}
        TArgumentPackBase(ArgumentPackVoid)
        {}

        template<typename CallbackT, typename ... ArgsT>
        typename CallbackT::ReturnType Invoke(CallbackT& callback, ArgsT&& ... args)
        {
            return callback.Invoke(std::forward<ArgsT>(args)...);
        }
    };

    template<typename ... ArgsT>
    struct TArgumentPack : public TArgumentPackBase<ArgsT..., ArgumentPackVoid>
    {
        using Super = TArgumentPackBase<ArgsT..., ArgumentPackVoid>;
        TArgumentPack(ArgsT ... args)
            : Super(std::forward<ArgsT>(args)..., ArgumentPackVoid())
        {}
    };

    // NOTE: This was derived from C++ MSVC, it might not necessarily hold true for other compilers
    LF_FORCE_INLINE UIntPtrT VTableFromThis(void* thisPtr)
    {
        return reinterpret_cast<UIntPtrT>((reinterpret_cast<void**>(thisPtr))[0]);
    }

    template<typename ReturnT, SizeT CBufferSize, typename ... ArgsT>
    class TCallbackBase
    {
    public:
        // ** Base class for invoking 
        struct BaseInvoke
        {
            virtual ~BaseInvoke() {}
            virtual ReturnT Invoke(ArgsT&&... args) const = 0;
            virtual BaseInvoke* CopyTo(ByteT(&buffer)[CBufferSize]) const = 0;
            virtual bool IsValid() const = 0;
        };

        struct BaseMethodInvoke : public BaseInvoke
        {
            virtual ~BaseMethodInvoke() {}
            virtual void UnbindObject() = 0;
        };

        template<typename LambdaT>
        struct LambdaType : public BaseInvoke
        {
            using Type = LambdaT;

            LambdaType(Type function)
                : mFunction(function)
            {}

            ReturnT Invoke(ArgsT&& ... args) const override
            {
                return mFunction(std::forward<ArgsT>(args)...);
            }

            BaseInvoke* CopyTo(ByteT(&buffer)[CBufferSize]) const override
            {
                return new (buffer) LambdaType(mFunction);
            }

            bool IsValid() const override
            {
                return true;
            }

            Type mFunction;
        };

        struct FunctionType : public BaseInvoke
        {
            using Type = ReturnT(*)(ArgsT...);
            FunctionType(Type function)
                : mFunction(function)
            {}

            ReturnT Invoke(ArgsT&& ... args) const override
            {
                return mFunction(std::forward<ArgsT>(args)...);
            }

            BaseInvoke* CopyTo(ByteT(&buffer)[CBufferSize]) const override
            {
                return new (buffer) FunctionType(mFunction);
            }

            bool IsValid() const override
            {
                return mFunction;
            }

            Type mFunction;
        };

        template<typename T>
        struct MethodType : public BaseMethodInvoke
        {
            using Type = ReturnT(T::*)(ArgsT...);
            using PointerType = T*;

            MethodType(PointerType object, Type function)
                : mFunction(function)
                , mObject(object)
            {
            }

            ReturnT Invoke(ArgsT&& ... args) const override
            {
                return ((*mObject).*mFunction)(std::forward<ArgsT>(args)...);
            }

            BaseInvoke* CopyTo(ByteT(&buffer)[CBufferSize]) const override
            {
                return new (buffer) MethodType(mObject, mFunction);
            }

            bool IsValid() const override
            {
                return mFunction && mObject;
            }

            void UnbindObject() override
            {
                mObject = nullptr;
            }

            static UIntPtrT GetVTableAddress()
            {
                MethodType instance(nullptr, nullptr);
                return VTableFromThis(&instance);
            }

            Type        mFunction;
            PointerType mObject;
        };
        template<typename T>
        struct MethodType < TWeakPointer<T> > : public BaseMethodInvoke
        {
            using Type = ReturnT(T::*)(ArgsT...);
            using PointerType = TWeakPointer<T>;

            MethodType(PointerType object, Type function)
                : mFunction(function)
                , mObject(object)
            {
            }

            ReturnT Invoke(ArgsT&& ... args) const override
            {
                return ((*mObject).*mFunction)(std::forward<ArgsT>(args)...);
            }

            BaseInvoke* CopyTo(ByteT(&buffer)[CBufferSize]) const override
            {
                return new (buffer) MethodType(mObject, mFunction);
            }

            bool IsValid() const override
            {
                return mFunction && mObject;
            }

            void UnbindObject() override
            {
                mObject = NULL_PTR;
            }

            static UIntPtrT GetVTableAddress()
            {
                MethodType instance(PointerType(), nullptr);
                return VTableFromThis(&instance);
            }

            Type                mFunction;
            mutable PointerType mObject;
        };

        template<typename T>
        struct MethodType < TAtomicWeakPointer<T> > : public BaseMethodInvoke
        {
            using Type = ReturnT(T::*)(ArgsT...);
            using PointerType = TAtomicWeakPointer<T>;

            MethodType(PointerType object, Type function)
                : mFunction(function)
                , mObject(object)
            {
            }

            ReturnT Invoke(ArgsT&& ... args) const override
            {
                return ((*mObject).*mFunction)(std::forward<ArgsT>(args)...);
            }

            BaseInvoke* CopyTo(ByteT(&buffer)[CBufferSize]) const override
            {
                return new (buffer) MethodType(mObject, mFunction);
            }

            bool IsValid() const override
            {
                return mFunction && mObject;
            }

            void UnbindObject() override
            {
                mObject = NULL_PTR;
            }

            static UIntPtrT GetVTableAddress()
            {
                MethodType instance(PointerType(), nullptr);
                return VTableFromThis(&instance);
            }

            Type                mFunction;
            mutable PointerType mObject;
        };
        template<typename T>
        struct ConstMethodType : public BaseMethodInvoke
        {
            using Type = ReturnT(T::*)(ArgsT...) const;
            using PointerType = const T*;

            ConstMethodType(PointerType object, Type function)
                : mFunction(function)
                , mObject(object)
            {
            }

            ReturnT Invoke(ArgsT&& ... args) const override
            {
                return ((*mObject).*mFunction)(std::forward<ArgsT>(args)...);
            }

            BaseInvoke* CopyTo(ByteT(&buffer)[CBufferSize]) const override
            {
                return new (buffer) ConstMethodType(mObject, mFunction);
            }

            bool IsValid() const override
            {
                return mFunction && mObject;
            }

            void UnbindObject() override
            {
                mObject = nullptr;
            }

            static UIntPtrT GetVTableAddress()
            {
                ConstMethodType instance(nullptr, nullptr);
                return VTableFromThis(&instance);
            }

            Type        mFunction;
            PointerType mObject;
        };
        template<typename T>
        struct ConstMethodType < TWeakPointer<T> > : public BaseMethodInvoke
        {
            using Type = ReturnT(T::*)(ArgsT...) const;
            using PointerType = TWeakPointer<T>;

            ConstMethodType(PointerType object, Type function)
                : mFunction(function)
                , mObject(object)
            {
            }

            ReturnT Invoke(ArgsT&& ... args) const override
            {
                return ((*mObject).*mFunction)(std::forward<ArgsT>(args)...);
            }

            BaseInvoke* CopyTo(ByteT(&buffer)[CBufferSize]) const override
            {
                return new (buffer) ConstMethodType(mObject, mFunction);
            }

            bool IsValid() const override
            {
                return mFunction && mObject;
            }

            void UnbindObject() override
            {
                mObject = NULL_PTR;
            }

            static UIntPtrT GetVTableAddress()
            {
                ConstMethodType instance(PointerType(), nullptr);
                return VTableFromThis(&instance);
            }

            Type                mFunction;
            mutable PointerType mObject;
        };

        template<typename T>
        struct ConstMethodType < TAtomicWeakPointer<T> > : public BaseMethodInvoke
        {
            using Type = ReturnT(T::*)(ArgsT...) const;
            using PointerType = TAtomicWeakPointer<T>;

            ConstMethodType(PointerType object, Type function)
                : mFunction(function)
                , mObject(object)
            {
            }

            ReturnT Invoke(ArgsT&& ... args) const override
            {
                return ((*mObject).*mFunction)(std::forward<ArgsT>(args)...);
            }

            BaseInvoke* CopyTo(ByteT(&buffer)[CBufferSize]) const override
            {
                return new (buffer) ConstMethodType(mObject, mFunction);
            }

            bool IsValid() const override
            {
                return mFunction && mObject;
            }

            void UnbindObject() override
            {
                mObject = NULL_PTR;
            }

            static UIntPtrT GetVTableAddress()
            {
                ConstMethodType instance(PointerType(), nullptr);
                return VTableFromThis(&instance);
            }

            Type                mFunction;
            mutable PointerType mObject;
        };

        using ReturnType = ReturnT;
        using ArgsType = TArgumentPack<ArgsT...>;

        TCallbackBase()
            : mType(CallbackType::INVALID_ENUM)
            , mInvoker(nullptr)
            , mData{ 0 }
        {
            LF_STATIC_ASSERT(CBufferSize >= 16);
        }

        TCallbackBase(const TCallbackBase& other)
            : mType(other.mType)
            , mInvoker(nullptr)
            , mData{ 0 }
        {
            if (other.mInvoker)
            {
                mInvoker = other.mInvoker->CopyTo(mData);
            }
        }

        TCallbackBase(TCallbackBase&& other)
            : mType(CallbackType::INVALID_ENUM)
            , mInvoker(nullptr)
            , mData{ 0 }
        {
            if (other.mInvoker)
            {
                mType = other.mType;
                other.mType = CallbackType::INVALID_ENUM;
                memcpy(mData, other.mData, CBufferSize);
                mInvoker = reinterpret_cast<BaseInvoke*>(mData);
                other.mInvoker = nullptr;
                memset(other.mData, 0, CBufferSize);
            }
        }

        ~TCallbackBase()
        {
            Release();
        }

        TCallbackBase& operator=(const TCallbackBase& other)
        {
            if (this == &other)
            {
                return *this;
            }

            Release();

            if (other.mInvoker)
            {
                mType = other.mType;
                mInvoker = other.mInvoker->CopyTo(mData);
            }
            return *this;
        }

        TCallbackBase& operator=(TCallbackBase&& other)
        {
            if (this == &other)
            {
                return *this;
            }

            Release();

            if (other.mInvoker)
            {
                mType = other.mType;
                other.mType = CallbackType::INVALID_ENUM;
                memcpy(mData, other.mData, CBufferSize);
                mInvoker = reinterpret_cast<BaseInvoke*>(mData);
                other.mInvoker = nullptr;
                memset(other.mData, 0, CBufferSize);
            }

            return *this;
        }


        static TCallbackBase Make(typename FunctionType::Type function)
        {
            TCallbackBase callback;
            callback.mInvoker = new (callback.mData) FunctionType(function);
            callback.mType = CallbackType::CT_FUNCTION;
            CriticalAssert(reinterpret_cast<void*>(callback.mInvoker) == reinterpret_cast<void*>(callback.mData));
            return callback;
        }

        template<typename LambdaT, SizeT CLambdaSize = sizeof(LambdaT)>
        static TCallbackBase Make(LambdaT function)
        {
            LF_STATIC_ASSERT(sizeof(LambdaT) <= CBufferSize);
            TCallbackBase callback;
            callback.mInvoker = new (callback.mData) LambdaType<LambdaT>(function);
            callback.mType = CallbackType::CT_LAMBDA;
            CriticalAssert(reinterpret_cast<void*>(callback.mInvoker) == reinterpret_cast<void*>(callback.mData));
            return callback;
        }

        template<typename T>
        static TCallbackBase Make(T* object, typename MethodType<T>::Type function)
        {
            TCallbackBase callback;
            callback.mInvoker = new (callback.mData) MethodType<T>(object, function);
            callback.mType = CallbackType::CT_METHOD;
            CriticalAssert(reinterpret_cast<void*>(callback.mInvoker) == reinterpret_cast<void*>(callback.mData));
            return callback;
        }

        template<typename T>
        static TCallbackBase Make(const TWeakPointer<T>& object, typename MethodType<T>::Type function)
        {
            TCallbackBase callback;
            callback.mInvoker = new (callback.mData) MethodType<TWeakPointer<T>>(object, function);
            callback.mType = CallbackType::CT_WEAK_PTR_METHOD;
            CriticalAssert(reinterpret_cast<void*>(callback.mInvoker) == reinterpret_cast<void*>(callback.mData));
            return callback;
        }

        template<typename T>
        static TCallbackBase Make(const TStrongPointer<T>& object, typename MethodType<T>::Type function)
        {
            return Make<T>(TWeakPointer<T>(object), function);
        }

        template<typename T>
        static TCallbackBase Make(const TAtomicWeakPointer<T>& object, typename MethodType<T>::Type function)
        {
            TCallbackBase callback;
            callback.mInvoker = new (callback.mData) MethodType<TAtomicWeakPointer<T>>(object, function);
            callback.mType = CallbackType::CT_ATOMIC_WEAK_PTR_METHOD;
            CriticalAssert(reinterpret_cast<void*>(callback.mInvoker) == reinterpret_cast<void*>(callback.mData));
            return callback;
        }

        template<typename T>
        static TCallbackBase Make(const TAtomicStrongPointer<T>& object, typename MethodType<T>::Type function)
        {
            return Make<T>(TAtomicWeakPointer<T>(object), function);
        }

        template<typename T>
        static TCallbackBase Make(const T* object, typename ConstMethodType<T>::Type function)
        {
            TCallbackBase callback;
            callback.mInvoker = new (callback.mData) ConstMethodType<T>(object, function);
            callback.mType = CallbackType::CT_CONST_METHOD;
            CriticalAssert(reinterpret_cast<void*>(callback.mInvoker) == reinterpret_cast<void*>(callback.mData));
            return callback;
        }

        template<typename T>
        static TCallbackBase Make(const TWeakPointer<T>& object, typename ConstMethodType<T>::Type function)
        {
            TCallbackBase callback;
            callback.mInvoker = new (callback.mData) ConstMethodType<TWeakPointer<T>>(object, function);
            callback.mType = CallbackType::CT_WEAK_PTR_CONST_METHOD;
            CriticalAssert(reinterpret_cast<void*>(callback.mInvoker) == reinterpret_cast<void*>(callback.mData));
            return callback;
        }

        template<typename T>
        static TCallbackBase Make(const TStrongPointer<T>& object, typename ConstMethodType<T>::Type function)
        {
            return Make<T>(TWeakPointer<T>(object), function);
        }

        template<typename T>
        static TCallbackBase Make(const TAtomicWeakPointer<T>& object, typename ConstMethodType<T>::Type function)
        {
            TCallbackBase callback;
            callback.mInvoker = new (callback.mData) ConstMethodType<TAtomicWeakPointer<T>>(object, function);
            callback.mType = CallbackType::CT_ATOMIC_WEAK_PTR_CONST_METHOD;
            CriticalAssert(reinterpret_cast<void*>(callback.mInvoker) == reinterpret_cast<void*>(callback.mData));
            return callback;
        }

        template<typename T>
        static TCallbackBase Make(const TAtomicStrongPointer<T>& object, typename ConstMethodType<T>::Type function)
        {
            return Make<T>(TAtomicWeakPointer<T>(object), function);
        }

        ReturnT Invoke(ArgsT ... args) const
        {
            return mInvoker->Invoke(std::forward<ArgsT>(args)...);
        }

        ReturnT operator()(ArgsT ... args) const
        {
            return mInvoker->Invoke(std::forward<ArgsT>(args)...);
        }

        bool operator==(const TCallbackBase& other) const
        {
            return mType == other.mType && memcmp(mData, other.mData, CBufferSize) == 0;
        }

        operator bool() const
        {
            return IsValid();
        }

        void Release()
        {
            if (mInvoker)
            {
                mInvoker->~BaseInvoke();
                mInvoker = nullptr;
                memset(mData, 0, CBufferSize);
                mType = CallbackType::INVALID_ENUM;
            }
        }

        bool IsMethod() const
        {
            switch (mType)
            {
            case CallbackType::CT_METHOD:
            case CallbackType::CT_WEAK_PTR_METHOD:
            case CallbackType::CT_ATOMIC_WEAK_PTR_METHOD:
                return true;
            default:
                break;
            }
            return false;
        }

        bool IsConstMethod() const
        {
            switch (mType)
            {
            case CallbackType::CT_CONST_METHOD:
            case CallbackType::CT_WEAK_PTR_CONST_METHOD:
            case CallbackType::CT_ATOMIC_WEAK_PTR_CONST_METHOD:
                return true;
            default:
                break;
            }
            return false;
        }

        bool IsFunction() const
        {
            return mType == CallbackType::CT_FUNCTION;
        }

        bool IsLambda() const
        {
            return mType == CallbackType::CT_LAMBDA;
        }

        bool IsValid() const
        {
            return mInvoker != nullptr && mInvoker->IsValid();
        }

        template<typename T>
        bool BindObject(T* object)
        {
            MethodType<T>* method = AsMethod<T>();
            if (method)
            {
                method->mObject = object;
                return true;
            }
            return false;
        }

        template<typename T>
        bool BindObject(const TWeakPointer<T>& object)
        {
            MethodType<TWeakPointer<T>>* method = AsMethod<TWeakPointer<T>>();
            if (method)
            {
                method->mObject = object;
                return true;
            }
            return false;
        }

        template<typename T>
        bool BindObject(const TStrongPointer<T>& object)
        {
            return BindObject<T>(TWeakPointer<T>(object));
        }

        template<typename T>
        bool BindObject(const TAtomicWeakPointer<T>& object)
        {
            MethodType<TAtomicWeakPointer<T>>* method = AsMethod<TAtomicWeakPointer<T>>();
            if (method)
            {
                method->mObject = object;
                return true;
            }
            return false;
        }

        template<typename T>
        bool BindObject(const TAtomicStrongPointer<T>& object)
        {
            return BindObject<T>(TAtomicWeakPointer<T>(object));
        }

        template<typename T>
        bool BindConstObject(const T* object)
        {
            ConstMethodType<T>* method = AsConstMethod<T>();
            if (method)
            {
                method->mObject = object;
                return true;
            }
            return false;
        }

        template<typename T>
        bool BindConstObject(const TWeakPointer<T>& object)
        {
            ConstMethodType<TWeakPointer<T>>* method = AsConstMethod<TWeakPointer<T>>();
            if (method)
            {
                method->mObject = object;
                return true;
            }
            return false;
        }

        template<typename T>
        bool BindConstObject(const TStrongPointer<T>& object)
        {
            return BindConstObject<T>(TWeakPointer<T>(object));
        }

        template<typename T>
        bool BindConstObject(const TAtomicWeakPointer<T>& object)
        {
            ConstMethodType<TAtomicWeakPointer<T>>* method = AsConstMethod<TAtomicWeakPointer<T>>();
            if (method)
            {
                method->mObject = object;
                return true;
            }
            return false;
        }

        template<typename T>
        bool BindConstObject(const TAtomicStrongPointer<T>& object)
        {
            return BindConstObject<T>(TAtomicWeakPointer<T>(object));
        }

        void UnbindObject()
        {
            if (IsMethod() || IsConstMethod())
            {
                static_cast<BaseMethodInvoke*>(mInvoker)->UnbindObject();
            }
        }

        typename FunctionType::Type GetFunctionPtr() const
        {
            if (!mInvoker || !IsFunction())
            {
                return nullptr;
            }
            return static_cast<FunctionType*>(mInvoker)->mFunction;
        }

        template<typename T>
        typename MethodType<T>::Type GetMethodPtr() const
        {
            if (!mInvoker || !IsMethod())
            {
                return nullptr;
            }
            return static_cast<MethodType<T>*>(mInvoker)->mFunction;
        }

        template<typename T>
        typename ConstMethodType<T>::Type GetConstMethodPtr() const
        {
            if (!mInvoker || !IsConstMethod())
            {
                return nullptr;
            }
            return static_cast<ConstMethodType<T>*>(mInvoker)->mFunction;
        }

    protected:
        UIntPtrT GetInvokerTypeAddress()
        {
            return mInvoker ? VTableFromThis(mInvoker) : 0;
        }

        template<typename T>
        MethodType<T>* AsMethod()
        {
            if (MethodType<T>::GetVTableAddress() != GetInvokerTypeAddress())
            {
                return nullptr;
            }
            return static_cast<MethodType<T>*>(mInvoker);
        }

        template<typename T>
        ConstMethodType<T>* AsConstMethod()
        {
            if (ConstMethodType<T>::GetVTableAddress() != GetInvokerTypeAddress())
            {
                return nullptr;
            }
            return static_cast<MethodType<T>*>(mInvoker);
        }

        CallbackType mType;
        BaseInvoke* mInvoker;
        ByteT mData[CBufferSize];
    };

    template<typename ReturnT, typename ... ArgsT>
    using TCallback = TCallbackBase<ReturnT, 64, ArgsT...>;

    template<FNV::HashT CHash, SizeT CBufferSize, typename ReturnT, typename ... ArgsT>
    class THashedCallbackBase : public TCallbackBase<ReturnT, CBufferSize, ArgsT...>
    {
    public:
        using Super = TCallbackBase<ReturnT, CBufferSize, ArgsT...>;
        using HashType = FNV::HashT;
        using AnonymousCallbackType = TAnonymousCallback<CBufferSize>;
        static const HashType HASH_VALUE = CHash;

        THashedCallbackBase() : Super()
        {}

        THashedCallbackBase(const Super& super) : Super(super)
        {}

        AnonymousCallbackType DownCast()
        {
            // Note: Callback has no way to know how to inc/dec ref on smart pointers.
            AnonymousCallbackType callback;
            memcpy(callback.mData, mData, CBufferSize);
            callback.mType = mType;
            callback.mSignatureHash = HASH_VALUE;
            return callback;
        }

        bool UpCast(const AnonymousCallbackType& callback)
        {
            // Note: Callback has no way to know how to inc/dec ref on smart pointers.
            if (callback.mSignatureHash != HASH_VALUE)
            {
                return false;
            }
            memcpy(mData, callback.mData, CBufferSize);
            mInvoker = reinterpret_cast<BaseInvoke*>(mData);
            mType = callback.mType;
            return true;
        }

        AnonymousCallback DownCastAnonymous()
        {
            LF_STATIC_ASSERT(sizeof(AnonymousCallback::mData) >= CBufferSize);
            AnonymousCallback callback;
            memcpy(callback.mData, mData, CBufferSize);
            callback.mType = mType;
            callback.mSignatureHash = HASH_VALUE;
            return callback;
        }

        bool UpCast(const AnonymousCallback& callback)
        {
            LF_STATIC_ASSERT(sizeof(AnonymousCallback::mData) >= CBufferSize);

            // Note: Callback has no way to know how to inc/dec ref on smart pointers.
            if (callback.mSignatureHash != HASH_VALUE)
            {
                return false;
            }
            memcpy(mData, callback.mData, CBufferSize);
            mInvoker = reinterpret_cast<BaseInvoke*>(mData);
            mType = callback.mType;
            return true;
        }

    };

    template<FNV::HashT CHash, typename ReturnT, typename ... ArgsT>
    using THashedCallback = THashedCallbackBase<CHash, 64, ReturnT, ArgsT...>;

} // namespace lf

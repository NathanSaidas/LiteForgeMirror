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

#ifndef LF_CORE_SMART_CALLBACK_H
#define LF_CORE_SMART_CALLBACK_H

#include "Core/Common/Types.h"
#include "Core/Common/Enum.h"

// =======================================================================
// ShieldHeart Smart Callback:
// Smart callback provides an interface for wrapping methods / functions
// into a common use interface.
//  
// Note: The return type for callbacks is limited to value types that are default constructible.
// =======================================================================

namespace lf
{
    DECLARE_STRICT_ENUM(CallbackType,
        CT_FUNCTION,
        CT_METHOD,
        CT_CONST_METHOD,
        CT_LAMBDA
        );

    template<typename T, typename ... ARGS>
    class TCallback;

    class CallbackHandle
    {
    public:
        static const SizeT DATA_SIZE = 64;
        typedef bool(*t_IsValidCallback)(const void*);
        typedef void(*t_WrapperCallback)(void*, void*);
        typedef void(*t_WrapperDestroyCallback)(void*);

        LF_INLINE CallbackHandle() :
            mType(CallbackType::INVALID_ENUM),
            mInvokeCallback(nullptr),
            mCopyCallback(nullptr),
            mMoveCallback(nullptr),
            mDestroyCallback(nullptr),
            mData()
        {
            memset(mData, 0, sizeof(mData));
        }

        LF_INLINE CallbackHandle(const CallbackHandle& other) :
            mType(other.mType),
            mInvokeCallback(other.mInvokeCallback),
            mIsValidCallback(other.mIsValidCallback),
            mCopyCallback(other.mCopyCallback),
            mMoveCallback(other.mMoveCallback),
            mDestroyCallback(other.mDestroyCallback),
            mData()
        {
            if (mCopyCallback)
            {
                mCopyCallback(mData, other.mData);
            }
        }

        LF_INLINE CallbackHandle(CallbackHandle&& other) :
            mType(std::move(other.mType)),
            mInvokeCallback(std::move(other.mInvokeCallback)),
            mIsValidCallback(std::move(other.mIsValidCallback)),
            mCopyCallback(std::move(other.mCopyCallback)),
            mMoveCallback(std::move(other.mMoveCallback)),
            mDestroyCallback(std::move(other.mDestroyCallback)),
            mData()
        {
            if (mMoveCallback)
            {
                mMoveCallback(mData, other.mData);
            }

            other.mType = CallbackType::INVALID_ENUM;
            other.mInvokeCallback = nullptr;
            other.mIsValidCallback = nullptr;
            other.mCopyCallback = nullptr;
            other.mMoveCallback = nullptr;
            other.mDestroyCallback = nullptr;
            memset(other.mData, 0, sizeof(other.mData));
        }

        LF_INLINE ~CallbackHandle()
        {
            if (mDestroyCallback)
            {
                mDestroyCallback(mData);
            }
        }

        // Called to assign the callback to this
        template<typename R, typename ... ARGS>
        void Assign(const TCallback<R, ARGS...>& callback) const;

        // Called to assign this to the callback
        template<typename R, typename ... ARGS>
        void Acquire(const TCallback<R, ARGS...>& callback) const;

        LF_INLINE CallbackHandle& operator=(const CallbackHandle& other)
        {
            if (mDestroyCallback)
            {
                mDestroyCallback(mData);
            }
            mType = other.mType;
            mIsValidCallback = other.mIsValidCallback;
            mInvokeCallback = other.mInvokeCallback;
            mCopyCallback = other.mCopyCallback;
            mMoveCallback = other.mMoveCallback;
            mDestroyCallback = other.mDestroyCallback;
            if (mCopyCallback)
            {
                mCopyCallback(mData, other.mData);
            }
            return(*this);
        }

        LF_INLINE CallbackHandle& operator=(CallbackHandle&& other)
        {
            if (mDestroyCallback)
            {
                mDestroyCallback(mData);
            }
            mType = std::move(other.mType);
            mInvokeCallback = std::move(other.mInvokeCallback);
            mIsValidCallback = std::move(other.mIsValidCallback);
            mCopyCallback = std::move(other.mCopyCallback);
            mMoveCallback = std::move(other.mMoveCallback);
            mDestroyCallback = std::move(other.mDestroyCallback);
            if (mMoveCallback)
            {
                mMoveCallback(mData, other.mData);
            }

            other.mType = CallbackType::INVALID_ENUM;
            other.mInvokeCallback = nullptr;
            other.mIsValidCallback = nullptr;
            other.mCopyCallback = nullptr;
            other.mMoveCallback = nullptr;
            other.mDestroyCallback = nullptr;
            memset(other.mData, 0, sizeof(other.mData));

            return(*this);
        }


        LF_INLINE bool IsValid() const
        {
            return IsLambda() || (mIsValidCallback && mIsValidCallback(mData));
        }

        LF_INLINE operator bool() const
        {
            return IsValid();
        }

        bool IsMethod() const
        {
            return mType == CallbackType::CT_METHOD || mType == CallbackType::CT_CONST_METHOD;
        }

        bool IsFunction() const
        {
            return mType == CallbackType::CT_FUNCTION;
        }

        bool IsLambda() const
        {
            return mType == CallbackType::CT_LAMBDA;
        }

    private:
        mutable CallbackType mType;
        mutable void* mInvokeCallback;
        mutable t_IsValidCallback mIsValidCallback;
        mutable t_WrapperCallback mCopyCallback;
        mutable t_WrapperCallback mMoveCallback;
        mutable t_WrapperDestroyCallback mDestroyCallback;
        mutable UInt8 mData[DATA_SIZE];
    };

    template<typename R, typename ... ARGS>
    class TCallback
    {
    public:
        static const SizeT DATA_SIZE = 64;
        typedef R(*t_InvokeCallback)(void*, ARGS ...);
        typedef bool(*t_IsValidCallback)(const void*);
        typedef void(*t_WrapperCallback)(void*, void*);
        typedef void(*t_WrapperDestroyCallback)(void*);

    private:
        class FunctionWrapper
        {
        public:
            typedef R(*t_Delegate)(ARGS...);
            LF_INLINE FunctionWrapper() : pointer(nullptr) {}
            LF_INLINE FunctionWrapper(const FunctionWrapper& other) : pointer(other.pointer) {}
            LF_INLINE FunctionWrapper(FunctionWrapper&& other) : pointer(std::move(other.pointer)) {}
            LF_INLINE ~FunctionWrapper() {}
            LF_INLINE bool IsValid() const { return pointer != nullptr; }
            LF_INLINE R Invoke(ARGS... args) const { return(pointer(args...)); }

            static R Invoke(void* data, ARGS ... args)
            {
                FunctionWrapper* wrapper = reinterpret_cast<FunctionWrapper*>(data);
                return(wrapper->Invoke(args...));
            }

            static void Copy(void* data, void* otherData)
            {
                FunctionWrapper* otherWrapper = reinterpret_cast<FunctionWrapper*>(otherData);
                FunctionWrapper* wrapper = new(data)FunctionWrapper(*otherWrapper);
                Assert(wrapper == data);
            }

            static void Move(void* data, void* otherData)
            {
                FunctionWrapper* otherWrapper = reinterpret_cast<FunctionWrapper*>(otherData);
                FunctionWrapper* wrapper = new(data)FunctionWrapper(std::move(*otherWrapper));
                Assert(wrapper == data);
            }

            static void Destroy(void* data)
            {
                FunctionWrapper* wrapper = reinterpret_cast<FunctionWrapper*>(data);
                wrapper->~FunctionWrapper();
            }

            static bool IsValid(const void* data)
            {
                const FunctionWrapper* wrapper = reinterpret_cast<const FunctionWrapper*>(data);
                return wrapper->IsValid();
            }

            t_Delegate pointer;
        };

        template<typename T>
        class MethodWrapper
        {
        public:
            typedef R(T::value_type::*t_Delegate)(ARGS...);
            LF_INLINE MethodWrapper() : pointer(nullptr), object() {}
            LF_INLINE MethodWrapper(const MethodWrapper& other) : pointer(other.pointer), object(other.object) {}
            LF_INLINE MethodWrapper(MethodWrapper&& other) : pointer(std::move(other.pointer)), object(std::move(other.object)) {}
            LF_INLINE ~MethodWrapper() {}
            LF_INLINE bool IsValid() const { return pointer != nullptr && object; }
            LF_INLINE R Invoke(ARGS... args) const { return(((*object)->*pointer)(args...)); }

            static R Invoke(void* data, ARGS ... args)
            {
                MethodWrapper* wrapper = reinterpret_cast<MethodWrapper*>(data);
                return(wrapper->Invoke(args...));
            }

            static void Copy(void* data, void* otherData)
            {
                MethodWrapper* otherWrapper = reinterpret_cast<MethodWrapper*>(otherData);
                MethodWrapper* wrapper = new(data)MethodWrapper(*otherWrapper);
                Assert(wrapper == data);
            }

            static void Move(void* data, void* otherData)
            {
                MethodWrapper* otherWrapper = reinterpret_cast<MethodWrapper*>(otherData);
                MethodWrapper* wrapper = new(data)MethodWrapper(std::move(*otherWrapper));
                Assert(wrapper == data);
            }

            static void Destroy(void* data)
            {
                MethodWrapper* wrapper = reinterpret_cast<MethodWrapper*>(data);
                wrapper->~MethodWrapper();
            }

            static bool IsValid(const void* data)
            {
                const MethodWrapper* wrapper = reinterpret_cast<const MethodWrapper*>(data);
                return wrapper->IsValid();
            }

            t_Delegate pointer;
            T object;
        };

        template<typename T>
        class ConstMethodWrapper
        {
        public:
            typedef R(T::value_type::*t_Delegate)(ARGS...) const;
            LF_INLINE ConstMethodWrapper() : pointer(nullptr), object() {}
            LF_INLINE ConstMethodWrapper(const ConstMethodWrapper& other) : pointer(other.pointer), object(other.object) {}
            LF_INLINE ConstMethodWrapper(ConstMethodWrapper&& other) : pointer(std::move(other.pointer)), object(std::move(other.object)) {}
            LF_INLINE ~ConstMethodWrapper() {}
            LF_INLINE bool IsValid() const { return pointer != nullptr && object; }
            LF_INLINE R Invoke(ARGS... args) const { return(((*object)->*pointer)(args...)); }

            static R Invoke(void* data, ARGS ... args)
            {
                ConstMethodWrapper* wrapper = reinterpret_cast<ConstMethodWrapper*>(data);
                return(wrapper->Invoke(args...));
            }

            static void Copy(void* data, void* otherData)
            {
                ConstMethodWrapper* otherWrapper = reinterpret_cast<ConstMethodWrapper*>(otherData);
                ConstMethodWrapper* wrapper = new(data)ConstMethodWrapper(*otherWrapper);
                Assert(wrapper == data);
            }

            static void Move(void* data, void* otherData)
            {
                ConstMethodWrapper* otherWrapper = reinterpret_cast<ConstMethodWrapper*>(otherData);
                ConstMethodWrapper* wrapper = new(data)ConstMethodWrapper(std::move(*otherWrapper));
                Assert(wrapper == data);
            }

            static void Destroy(void* data)
            {
                ConstMethodWrapper* wrapper = reinterpret_cast<ConstMethodWrapper*>(data);
                wrapper->~ConstMethodWrapper();
            }

            static bool IsValid(const void* data)
            {
                const ConstMethodWrapper* wrapper = reinterpret_cast<const ConstMethodWrapper*>(data);
                return wrapper->IsValid();
            }

            t_Delegate pointer;
            T object;
        };

        template<typename LAMBDA>
        class LambdaWrapper
        {
        public:
            typedef LAMBDA t_Delegate;
            LF_INLINE LambdaWrapper(const LambdaWrapper& other) : pointer(other.pointer) {}
            LF_INLINE LambdaWrapper(LambdaWrapper&& other) : pointer(std::move(other.pointer)) {}
            LF_INLINE LambdaWrapper(const LAMBDA& lambda) : pointer(lambda) {}
            // LF_INLINE LambdaWrapper(LAMBDA&& lambda) : pointer(std::forward<LAMBDA>(lambda)) {}
            LF_INLINE ~LambdaWrapper() {}
            LF_INLINE bool IsValid() const { return true; }
            LF_INLINE R Invoke(ARGS... args) const
            {
                return(pointer(args...));
            }

            static R Invoke(void* data, ARGS ... args)
            {
                LambdaWrapper* wrapper = reinterpret_cast<LambdaWrapper*>(data);
                return(wrapper->Invoke(args...));
            }

            static void Copy(void* data, void* otherData)
            {
                LambdaWrapper* otherWrapper = reinterpret_cast<LambdaWrapper*>(otherData);
                LambdaWrapper* wrapper = new(data)LambdaWrapper(*otherWrapper);
                Assert(wrapper == data);
            }

            static void Move(void* data, void* otherData)
            {
                LambdaWrapper* otherWrapper = reinterpret_cast<LambdaWrapper*>(otherData);
                LambdaWrapper* wrapper = new(data)LambdaWrapper(std::move(*otherWrapper));
                Assert(wrapper == data);
            }

            static void Destroy(void* data)
            {
                LambdaWrapper* wrapper = reinterpret_cast<LambdaWrapper*>(data);
                wrapper->~LambdaWrapper();
            }

            t_Delegate pointer;
        };
    public:

        TCallback() :
            mType(CallbackType::INVALID_ENUM),
            mInvokeCallback(nullptr),
            mIsValidCallback(nullptr),
            mCopyCallback(nullptr),
            mMoveCallback(nullptr),
            mDestroyCallback(nullptr),
            mData()
        {

        }

        TCallback(const TCallback& other) :
            mType(other.mType),
            mInvokeCallback(other.mInvokeCallback),
            mIsValidCallback(other.mIsValidCallback),
            mCopyCallback(other.mCopyCallback),
            mMoveCallback(other.mMoveCallback),
            mDestroyCallback(other.mDestroyCallback),
            mData()
        {
            Copy(other);
        }

        TCallback(TCallback&& other) :
            mType(other.mType),
            mInvokeCallback(std::move(other.mInvokeCallback)),
            mIsValidCallback(std::move(other.mIsValidCallback)),
            mCopyCallback(std::move(other.mCopyCallback)),
            mMoveCallback(std::move(other.mMoveCallback)),
            mDestroyCallback(std::move(other.mDestroyCallback)),
            mData()
        {
            Move(std::forward<TCallback>(other));
        }

        ~TCallback()
        {
            Destroy();
        }

        // function ctor
        TCallback(typename FunctionWrapper::t_Delegate function)
        {
            BindFunction(function);
        }

        // method ctor
        template<typename T>
        TCallback(typename MethodWrapper<T>::t_Delegate method, const T& obj)
        {
            BindMethod(obj, method);
        }

        // method const ctor
        template<typename T>
        TCallback(typename ConstMethodWrapper<T>::t_Delegate method, const T& obj)
        {
            BindConstMethod(obj, method);
        }

        template<typename LAMBDA>
        static TCallback CreateLambda(const LAMBDA& lambda)
        {
            TCallback callback;
            callback.Bind(lambda);
            return callback;
        }

        // invoke
        R Invoke(ARGS ... args) const
        {
            return(mInvokeCallback(mData, args...));
        }

        bool IsValid() const
        {
            return(IsLambda() || (mIsValidCallback && mIsValidCallback(mData)));
        }

        void Bind(typename FunctionWrapper::t_Delegate function)
        {
            Destroy();
            BindFunction(function);
        }

        template<typename T>
        void Bind(const T& obj, typename MethodWrapper<T>::t_Delegate method)
        {
            Destroy();
            BindMethod(method);
        }

        template<typename T>
        void Bind(const T& obj, typename ConstMethodWrapper<T>::t_Delegate method)
        {
            Destroy();
            BindConstMethod(method);
        }

        template<typename LAMBDA>
        void Bind(const LAMBDA& lambda)
        {
            Destroy();
            BindLambda(lambda);
        }

        template<typename T>
        void BindObject(const T& obj)
        {
            if (mType == CallbackType::CT_METHOD)
            {
                MethodWrapper<T>* wrapper = static_cast<MethodWrapper<T>*>(static_cast<void*>(mData));
                wrapper->object = obj;
            }
            else if (mType == CallbackType::CT_CONST_METHOD)
            {
                ConstMethodWrapper<T>* wrapper = static_cast<ConstMethodWrapper<T>*>(static_cast<void*>(mData));
                wrapper->object = obj;
            }
        }

        template<typename T>
        void GetObject(T& obj)
        {
            if (mType == CallbackType::CT_METHOD)
            {
                MethodWrapper<T>* wrapper = static_cast<MethodWrapper<T>*>(static_cast<void*>(mData));
                obj = wrapper->object;
            }
            else if (mType == CallbackType::CT_CONST_METHOD)
            {
                ConstMethodWrapper<T>* wrapper = static_cast<ConstMethodWrapper<T>*>(static_cast<void*>(mData));
                obj = wrapper->object;
            }
        }

        TCallback& operator=(const TCallback& other)
        {
            Destroy();
            mType = other.mType;
            mInvokeCallback = other.mInvokeCallback;
            mIsValidCallback = other.mIsValidCallback;
            mCopyCallback = other.mCopyCallback;
            mMoveCallback = other.mMoveCallback;
            mDestroyCallback = other.mDestroyCallback;
            Copy(other);
            return(*this);
        }

        TCallback& operator=(TCallback&& other)
        {
            Destroy();
            mType = other.mType;
            mInvokeCallback = std::move(other.mInvokeCallback);
            mIsValidCallback = std::move(other.mIsValidCallback);
            mCopyCallback = std::move(other.mCopyCallback);
            mMoveCallback = std::move(other.mMoveCallback);
            mDestroyCallback = std::move(other.mDestroyCallback);
            Move(std::forward<TCallback>(other));
            return(*this);
        }

        operator bool() const
        {
            return IsValid();
        }

        void Assign(const CallbackHandle& handle)
        {
            Destroy();
            handle.Acquire(*this);
        }

        CallbackHandle GetHandle() const
        {
            CallbackHandle handle;
            handle.Assign(*this);
            return handle;
        }

        bool IsMethod() const
        {
            return mType == CallbackType::CT_METHOD || mType == CallbackType::CT_CONST_METHOD;
        }

        bool IsFunction() const
        {
            return mType == CallbackType::CT_FUNCTION;
        }

        bool IsLambda() const
        {
            return mType == CallbackType::CT_LAMBDA;
        }

    private:
        void BindFunction(typename FunctionWrapper::t_Delegate function)
        {
            LF_STATIC_ASSERT(sizeof(FunctionWrapper) < DATA_SIZE);
            FunctionWrapper* wrapper = new(mData)FunctionWrapper();
            Assert(wrapper == static_cast<void*>(mData));
            wrapper->pointer = function;
            mInvokeCallback = FunctionWrapper::Invoke;
            mIsValidCallback = FunctionWrapper::IsValid;
            mCopyCallback = FunctionWrapper::Copy;
            mMoveCallback = FunctionWrapper::Move;
            mDestroyCallback = FunctionWrapper::Destroy;
            mType = CallbackType::CT_FUNCTION;
        }

        template<typename T>
        void BindMethod(const T& obj, typename MethodWrapper<T>::t_Delegate method)
        {
            LF_STATIC_ASSERT(sizeof(MethodWrapper<T>) < DATA_SIZE);
            MethodWrapper<T>* wrapper = new(mData)MethodWrapper<T>();
            Assert(wrapper == static_cast<void*>(mData));
            wrapper->pointer = method;
            wrapper->object = obj;
            mInvokeCallback = MethodWrapper<T>::Invoke;
            mIsValidCallback = MethodWrapper<T>::IsValid;
            mCopyCallback = MethodWrapper<T>::Copy;
            mMoveCallback = MethodWrapper<T>::Move;
            mDestroyCallback = MethodWrapper<T>::Destroy;
            mType = CallbackType::CT_METHOD;
        }

        template<typename T>
        void BindConstMethod(const T& obj, typename ConstMethodWrapper<T>::t_Delegate method)
        {
            LF_STATIC_ASSERT(sizeof(ConstMethodWrapper<T>) < DATA_SIZE);
            ConstMethodWrapper<T>* wrapper = new(mData)ConstMethodWrapper<T>();
            Assert(wrapper == static_cast<void*>(mData));
            wrapper->pointer = method;
            wrapper->object = obj;
            mInvokeCallback = ConstMethodWrapper<T>::Invoke;
            mIsValidCallback = ConstMethodWrapper<T>::IsValid;
            mCopyCallback = ConstMethodWrapper<T>::Copy;
            mMoveCallback = ConstMethodWrapper<T>::Move;
            mDestroyCallback = ConstMethodWrapper<T>::Destroy;
            mType = CallbackType::CT_CONST_METHOD;
        }

        template<typename LAMBDA>
        void BindLambda(const LAMBDA& lambda)
        {
            // If this trips, either the amount of data you're capturing in the lambda is too much or were not reserving enough at static time.
            LF_STATIC_ASSERT(sizeof...(ARGS) < DATA_SIZE);
            LambdaWrapper<LAMBDA>* wrapper = new(mData)LambdaWrapper<LAMBDA>(lambda);
            Assert(wrapper == static_cast<void*>(mData));
            mInvokeCallback = LambdaWrapper<LAMBDA>::Invoke;
            mIsValidCallback = nullptr;
            mCopyCallback = LambdaWrapper<LAMBDA>::Copy;
            mMoveCallback = LambdaWrapper<LAMBDA>::Move;
            mDestroyCallback = LambdaWrapper<LAMBDA>::Destroy;
            mType = CallbackType::CT_LAMBDA;
        }

        void Copy(const TCallback& other)
        {
            if (mCopyCallback)
            {
                mCopyCallback(mData, const_cast<UInt8*>(other.mData));
            }
        }

        void Move(TCallback&& other)
        {
            if (mMoveCallback)
            {
                mMoveCallback(mData, other.mData);
            }

            other.mType = CallbackType::INVALID_ENUM;
            other.mInvokeCallback = nullptr;
            other.mIsValidCallback = nullptr;
            other.mCopyCallback = nullptr;
            other.mMoveCallback = nullptr;
            other.mDestroyCallback = nullptr;
            memset(other.mData, 0, sizeof(other.mData));
        }
        void Destroy()
        {
            if (mDestroyCallback)
            {
                mDestroyCallback(mData);
            }
        }

        mutable CallbackType mType;
        mutable t_InvokeCallback mInvokeCallback;
        mutable t_IsValidCallback mIsValidCallback;
        mutable t_WrapperCallback mCopyCallback;
        mutable t_WrapperCallback mMoveCallback;
        mutable t_WrapperDestroyCallback mDestroyCallback;
        mutable UInt8 mData[DATA_SIZE];

        friend class CallbackHandle;
    };


    template<typename R, typename ... ARGS>
    void CallbackHandle::Assign(const TCallback<R, ARGS...>& callback) const
    {
        if (mDestroyCallback)
        {
            mDestroyCallback(mData);
        }

        mType = callback.mType;
        mInvokeCallback = callback.mInvokeCallback;
        mIsValidCallback = callback.mIsValidCallback;
        mCopyCallback = callback.mCopyCallback;
        mMoveCallback = callback.mMoveCallback;
        mDestroyCallback = callback.mDestroyCallback;
        if (mCopyCallback)
        {
            mCopyCallback(mData, callback.mData);
        }
    }

    template<typename R, typename ... ARGS>
    void CallbackHandle::Acquire(const TCallback<R, ARGS...>& callback) const
    {
        callback.mType = mType;
        callback.mInvokeCallback = static_cast<typename TCallback<R, ARGS...>::t_InvokeCallback>(mInvokeCallback);
        callback.mIsValidCallback = mIsValidCallback;
        callback.mCopyCallback = mCopyCallback;
        callback.mMoveCallback = mMoveCallback;
        callback.mDestroyCallback = mDestroyCallback;
        if (callback.mCopyCallback)
        {
            callback.mCopyCallback(callback.mData, mData);
        }
    }
}

#endif // LF_CORE_SMART_CALLBACK_H
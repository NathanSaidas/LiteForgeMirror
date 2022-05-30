// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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
#include "Core/Common/API.h"
#include "Core/Common/Types.h"
#include "Core/Utility/Bitfield.h"

#include <new>

namespace lf {

class TempHeap;
class String;
struct StackTrace;

class LF_CORE_API ErrorBase
{
    static TempHeap* GetAllocator();
public:
    using ReportCallback = void(*)(const ErrorBase&, UInt32);
    struct LF_CORE_API Info
    {
        Info(const char* filename, SizeT line)
        : mFilename(filename)
        , mLine(line)
        , mFlags(ERROR_FLAG_LOG | ERROR_FLAG_LOG_CALLSTACK | ERROR_FLAG_LOG_THREAD)
        {}
        Info(const char* filename, SizeT line, UInt32 flags)
        : mFilename(filename)
        , mLine(line)
        , mFlags(flags)
        {}

        const char* mFilename;
        SizeT       mLine;
        UInt32      mFlags;
    };

    static void SetAllocator(TempHeap* heap);
    static ReportCallback SetReportCallback(ReportCallback callback);

    static void* BeginError(SizeT size, SizeT alignment);
    static void EndError();

    ErrorBase();
    ~ErrorBase();

    void Initialize(const char* file, SizeT line, UInt32 flags);
    void Release();

    void Ignore();
    void Report();

    const StackTrace& GetStackTrace() const;
    const char* GetFilename() const { return mFilename; }
    SizeT GetLine() const { return mLine; }

    virtual const String& GetErrorMessage() const = 0;
protected:
    void* Allocate(SizeT size, SizeT alignment);
    const char* GetUnknownErrorString() const;

private:
    struct LF_ALIGN(8) StackTraceBuffer
    {
        ByteT mBytes[16];
    };
    StackTraceBuffer mStackTraceBuffer;
    StackTrace* mStackTrace;
    const char* mFilename;
    UInt32 mFlags;
    UInt32 mLine;
    bool   mReleased;
};

namespace ErrorUtil
{
    template<typename T, typename ... ARGS>
    T* MakeError(const ErrorBase::Info& info, ARGS&&... args)
    {
        void* memory = ErrorBase::BeginError(sizeof(T), alignof(T));
        T* error = memory ? new(memory)T(args...) : nullptr;
        if (error)
        {
            error->Initialize(info.mFilename, info.mLine, info.mFlags);
        }
        else
        {
            ErrorBase::EndError();
        }
        return error;
    }
}


template<typename T>
class APIResult
{
    template<typename> friend class APIResult;

public:
    explicit APIResult(T value)
    : mValue(std::move(value))
    , mError(nullptr)
    {}
    explicit APIResult(T value, ErrorBase* error)
    : mValue(std::move(value))
    , mError(error)
    {

    }
    // Move error constructor
    template<typename U>
    explicit APIResult(T&& value, APIResult<U>& otherError)
    : mValue(std::move(value))
    , mError(otherError.GetError())
    {
        otherError.mError = nullptr;
    }
    APIResult(APIResult&& other)
        : mValue(std::move(other.mValue))
        , mError(other.mError)
    {
        other.mError = nullptr;
    }
    APIResult& operator=(APIResult&& other)
    {
        mValue = std::move(other.mValue);
        mError = other.mError; other.mError = nullptr;
        return *this;
    }
    ~APIResult()
    {
        if (mError != nullptr)
        {
            Report();
        }
    }

    T& GetItem() { return mValue; }
    operator T() { return mValue; }
    ErrorBase* GetError() { return mError; }

    // Clears the error, it will not report on destructor
    void Ignore() { if (mError) { mError->Ignore(); mError = nullptr; ErrorBase::EndError(); } }
    // Report the error writing it to the log and then clearing it from this API Result.
    void Report() { if (mError) { mError->Report(); mError = nullptr; ErrorBase::EndError(); } }
private:

    APIResult() = delete;
    APIResult(const APIResult&) = delete;
    APIResult& operator=(const APIResult&) = delete;

    T mValue;
    ErrorBase* mError;
};

#if defined(ReportError)
#error "ReportError already defined!"
#endif
#define ReportError(ResultValue_, ErrorType_, ...) ::lf::APIResult<decltype(ResultValue_)>(ResultValue_, ErrorType_::Create(ErrorBase::Info(__FILE__, __LINE__, ERROR_FLAG_LOG | ERROR_FLAG_LOG_CALLSTACK | ERROR_FLAG_LOG_THREAD),__VA_ARGS__))

#if defined(ReportErrorWithFlags)
#error "ReportErrorWithFlags already defined!"
#endif
#define ReportErrorWithFlags(ResultValue_, ErrorType_, Flags_, ...) lf::APIResult<decltype(ResultValue_)>(ResultValue_, ErrorType_::Create(ErrorBase::Info(__FILE__,__LINE__, Flags_),__VA_ARGS__))

} // namespace lf
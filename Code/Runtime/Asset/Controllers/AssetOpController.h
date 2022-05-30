#pragma once
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

#include "Core/Common/API.h"
#include "Core/Concurrent/TaskScheduler.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Platform/SpinLock.h"
#include "Core/Utility/Array.h"
#include "Core/Utility/SmartCallback.h"
#include "Runtime/Asset/AssetCommon.h"

namespace lf {

DECLARE_ATOMIC_PTR(AssetOp);
DECLARE_ATOMIC_WPTR(AssetOp);

// Operations will be stored in different 'lists'
// 
// Idle List 
// Execution List
// Waiting List
//
// Op::Execute -- Moves the operation from the 'Idle' list to the 'Execution' list.
// Op::Update -- Potentially move to the 'Wait List' if the operation is waiting on another operation.
// Op::OnComplete -- Call all the registered callbacks and possibly move something off the wait list and onto the execution list
// Op::OnComplete -- Also moving the op off the execution list and ending it

class LF_RUNTIME_API AssetOpController
{
public:
    AssetOpController();
    ~AssetOpController();

    void Initialize();
    void Shutdown();

    void Update();

    void Register(AssetOp* op);

    template<typename T>
    void Call(AssetOpThread::Value thread, const T& function, void* param = nullptr) { Call(thread, TCallback<void, void*>::Make(function), param); }
    void Call(AssetOpThread::Value thread, const TCallback<void, void*>& function, void* param = nullptr);

    AssetOpAtomicWPtr GetCompleted() const { return mCompletedOp; }

private:
    void DispatchAsyncCalls();

    SpinLock                mAsyncCallLock;
    TaskScheduler           mScheduler;
    struct AsyncCall
    {
        TCallback<void, void*> mFunction;
        void*                  mParam;
    };
    TVector<AsyncCall>        mMainThreadAsyncCalls;

    SpinLock                 mInitializeOpsLock;
    TVector<AssetOpAtomicPtr> mInitializeOps;

    volatile Atomic32        mAsyncOps;
    TVector<AssetOpAtomicPtr> mOps;

    AssetOpAtomicPtr         mCompletedOp;
};

} // namespace lf
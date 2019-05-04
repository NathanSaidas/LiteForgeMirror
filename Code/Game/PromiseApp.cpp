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
#include "Engine/App/Application.h"
#include "Engine/App/Program.h"
#include "Core/Concurrent/TaskScheduler.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Utility/SmartCallback.h"
#include "Core/Utility/Log.h"

#include "Runtime/Async/PromiseImpl.h"

using namespace lf;

void OnResolve()
{
    gSysLog.Info(LogMessage("Promise Resolved!"));
}

void OnError(const String& error)
{
    gSysLog.Error(LogMessage("Promise Error! Error=") << error);
}

class PromiseApp : public Application
{
    DECLARE_CLASS(PromiseApp, Application);
public:
    using MyPromiseType = PromiseImpl<TCallback<void>, TCallback<void, const String&>>;

    void RunPromise()
    {
        auto promise = MyPromiseType([](Promise* self)
        {
            self->Resolve();
            // self->Reject("Rejected!");
        })
        .Then(OnResolve)
        .Then([]() { gSysLog.Info(LogMessage("On Lambda Resolved! ThreadId=") << GetPlatformThreadId()); })
        .Catch(OnError)
        .Catch([](const String& error) { gSysLog.Error(LogMessage("On Lambda Error! Error=") << error); })
        .Execute();

        gSysLog.Info(LogMessage("Waiting for promise to complete!"));
        // promise->Wait();
        // 
        // promise->SetState(Promise::PROMISE_NULL);
        // GetAsync().ExecutePromise(promise);
        // 
        promise->LazyWait();
        (promise);
        // SleepCallingThread(2500);
        LF_DEBUG_BREAK;
    }

    void OnStart() override
    {
        gSysLog.Info(LogMessage("Tada"));
        RunPromise();
    }
};
DEFINE_CLASS(PromiseApp) { NO_REFLECTION; }


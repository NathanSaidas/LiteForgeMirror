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


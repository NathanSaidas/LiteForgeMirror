#include "Engine/App/Application.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Platform/Thread.h"
#include "Core/Platform/ThreadSignal.h"
#include "Core/Concurrent/TaskScheduler.h"

using namespace lf;
class ThreadQueueApp : public Application
{
    DECLARE_CLASS(ThreadQueueApp, Application);
public:
    struct MyState
    {
        ThreadSignal mSignal;
    };

    static void MyThread(void* param)
    {
        MyState* state = static_cast<MyState*>(param);

        gSysLog.Info(LogMessage("MyThread::Waiting for signal."));
        state->mSignal.Wait();
    }

    void OnStart() override
    {
        gSysLog.Info(LogMessage("ThreadQueueApp::OnStart"));

        // MyState state;
        // Thread thread;
        // thread.Fork(MyThread, &state);
        // 
        // gSysLog.Info(LogMessage("Main going to sleep..."));
        // SleepCallingThread(2500);
        // state.mSignal.WakeOne();
        // thread.Join();

        TaskScheduler task;
        task.Initialize(true);
        SleepCallingThread(5000);
        task.Shutdown();
    }

    void OnExit() override
    {
        gSysLog.Info(LogMessage("ThreadQueueApp::OnExit"));
    }
};
DEFINE_CLASS(ThreadQueueApp) { NO_REFLECTION; }

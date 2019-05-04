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

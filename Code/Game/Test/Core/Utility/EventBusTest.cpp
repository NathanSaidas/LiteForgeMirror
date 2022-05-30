// ********************************************************************
// Copyright (c) 2022 Nathan Hanlan
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
#include "Core/Test/Test.h"
#include "Core/Utility/Utility.h"
#include "Core/Utility/EventBus.h"
#include "Core/Utility/SmartCallback.h"
#include "Core/Utility/Log.h"
#include "Core/String/String.h"

namespace lf {

struct TestEventBusStruct
{
    String data0;
    String data1;
};
using TestEventBusCallback = TCallback<void, const TestEventBusStruct&>;
using TestUserEventBusType = TUserKeyEventBusBase<int, TestEventBusCallback>;
using TestGeneratedEventBusType = TGeneratedKeyEventBusBase<TestEventBusCallback>;

struct TestEventBusClient
{
    TestEventBusClient() : clientID(-1), eventReceived(false) {}
    explicit TestEventBusClient(int value) : clientID(value), eventReceived(false) {}

    void OnEvent(const TestEventBusStruct& )
    {
        eventReceived = true;
    }

    void Register(TestUserEventBusType& eventBus)
    {
        eventBus.Register(clientID, TestEventBusCallback::Make(this, &TestEventBusClient::OnEvent));
    }

    void Unregister(TestUserEventBusType& eventBus)
    {
        eventBus.Unregister(clientID);
    }

    void Register(TestGeneratedEventBusType& eventBus)
    {
        busID = eventBus.Register(TestEventBusCallback::Make(this, &TestEventBusClient::OnEvent));
    }

    void Unregister(TestGeneratedEventBusType& eventBus)
    {
        eventBus.Unregister(busID);
    }

    int clientID;
    TestGeneratedEventBusType::IDType busID;
    bool eventReceived;
};

struct TestIllegalEventBusClient
{
    void TryRegister(const TestEventBusStruct& )
    {
        eventBus->Register(TestEventBusCallback::Make(this, &TestIllegalEventBusClient::TryEvent));
    }

    void TryUnregister(const TestEventBusStruct& )
    {
        eventBus->Unregister(busID);
    }

    void TryEvent(const TestEventBusStruct& event)
    {
        eventBus->Invoke(event);
    }

    TestGeneratedEventBusType::IDType busID;
    TestGeneratedEventBusType* eventBus;
};

REGISTER_TEST(GeneratedEventBusTest, "Core.Utility")
{
    SizeT bytesBefore = LFGetBytesAllocated();
    {
        TestGeneratedEventBusType eventBus;
        TestEventBusClient eventClients[] = {
            TestEventBusClient(0),
            TestEventBusClient(1),
            TestEventBusClient(2)
        };

        TestEventBusStruct eventData;
        eventData.data0 = "Test_Data_0";
        eventData.data1 = "Test_Data_1_Big_Heap_Alloc_Forced_String!";

        for (SizeT i = 0; i < LF_ARRAY_SIZE(eventClients); ++i)
        {
            TEST(eventClients[i].eventReceived == false);
            eventClients[i].Register(eventBus);
        }

        eventBus.Invoke(eventData);

        for (SizeT i = 0; i < LF_ARRAY_SIZE(eventClients); ++i)
        {
            TEST(eventClients[i].eventReceived == true);
        }
    }
    SizeT bytesAfter = LFGetBytesAllocated();
    TEST(bytesBefore == bytesAfter);
}

REGISTER_TEST(UserEventBusTest, "Core.Utility")
{
    SizeT bytesBefore = LFGetBytesAllocated();
    {
        TestUserEventBusType eventBus;
        TestEventBusClient eventClients[] = {
            TestEventBusClient(0),
            TestEventBusClient(1),
            TestEventBusClient(2)
        };

        TestEventBusStruct eventData;
        eventData.data0 = "Test_Data_0";
        eventData.data1 = "Test_Data_1_Big_Heap_Alloc_Forced_String!";

        for (SizeT i = 0; i < LF_ARRAY_SIZE(eventClients); ++i)
        {
            TEST(eventClients[i].eventReceived == false);
            eventClients[i].Register(eventBus);
        }

        eventBus.Invoke(eventData);

        for (SizeT i = 0; i < LF_ARRAY_SIZE(eventClients); ++i)
        {
            TEST(eventClients[i].eventReceived == true);
        }
    }
    SizeT bytesAfter = LFGetBytesAllocated();
    TEST(bytesBefore == bytesAfter);
}

REGISTER_TEST(BadEventHandlingTest, "Core.Utility")
{
#if defined(LF_DEBUG) || defined(LF_TEST)
    TestGeneratedEventBusType eventBus;
    TestIllegalEventBusClient eventClient;
    eventClient.eventBus = &eventBus;

    TestEventBusStruct eventData;
    eventData.data0 = "Test_Data_0";
    eventData.data1 = "Test_Data_1_Big_Heap_Alloc_Forced_String!";

    // Try register
    eventClient.busID = eventBus.Register(TestEventBusCallback::Make(&eventClient, &TestIllegalEventBusClient::TryRegister));
    TEST_CRITICAL_EXCEPTION(eventBus.Invoke(eventData));
    eventBus.Unregister(eventClient.busID);
    // Try unregister
    eventClient.busID = eventBus.Register(TestEventBusCallback::Make(&eventClient, &TestIllegalEventBusClient::TryUnregister));
    TEST_CRITICAL_EXCEPTION(eventBus.Invoke(eventData));
    eventBus.Unregister(eventClient.busID);
    // Try invoke
    eventClient.busID = eventBus.Register(TestEventBusCallback::Make(&eventClient, &TestIllegalEventBusClient::TryEvent));
    TEST_CRITICAL_EXCEPTION(eventBus.Invoke(eventData));
    eventBus.Unregister(eventClient.busID);
#else
    LF_LOG_WARN(gTestLog, "Skipping test because it relies on exception handling.");
#endif
}

REGISTER_TEST(SafePointerEventBusTest, "Core.Utility")
{
    TestGeneratedEventBusType eventBus;
    
    TStrongPointer<TestEventBusClient> eventClient(LFNew<TestEventBusClient>());
    

    TestEventBusStruct eventData;
    eventData.data0 = "Test_Data_0";
    eventData.data1 = "Test_Data_1_Big_Heap_Alloc_Forced_String!";

    eventClient->busID = eventBus.Register(TestEventBusCallback::Make(TWeakPointer<TestEventBusClient>(eventClient), &TestEventBusClient::OnEvent));
    eventClient = NULL_PTR;
    eventBus.Invoke(eventData);
}

} // namespace lf
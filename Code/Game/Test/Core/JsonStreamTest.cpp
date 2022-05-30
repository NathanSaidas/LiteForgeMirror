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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANYJsonStream CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ********************************************************************
#include "Core/Test/Test.h"
#include "Core/IO/JsonStream.h"
#include "Core/Utility/Log.h"
#include "Game/Test/TestUtils.h"

namespace lf {

REGISTER_TEST(JsonStream_ComplexWriteTest, "Core.IO")
{
    String expected = "{\"Struct\":{\"SimpleValue\":173829},\"StructArray\":[{\"SimpleValue\":1292},{\"SimpleValue\":-1292}],\"ValueArray\":[28131,-1828,1992921],\"Value\":1337}";

    DummyStruct data;
    data.mValue = 1337;
    data.mStruct.mSimpleValue = 173829;
    data.mValueArray.push_back(28131);
    data.mValueArray.push_back(-1828);
    data.mValueArray.push_back(1992921);
    data.mStructArray.push_back({ 1292 });
    data.mStructArray.push_back({ -1292 });

    String output;
    JsonStream ts(Stream::TEXT, &output, Stream::SM_WRITE);
    if (ts.GetMode() == Stream::SM_WRITE)
    {
        ts << data;
        ts.Close();
    }

    gTestLog.Info(LogMessage("Result=\n") << output);

    TEST(output == expected);
}

REGISTER_TEST(JsonStream_ComplexReadTest, "Core.IO")
{
    String expected = "{\"Struct\":{\"SimpleValue\":173829},\"StructArray\":[{\"SimpleValue\":1292},{\"SimpleValue\":-1292}],\"ValueArray\":[28131,-1828,1992921],\"Value\":1337}";


    DummyStruct data;
    data.mValue = 1337;
    data.mStruct.mSimpleValue = 173829;
    data.mValueArray.push_back(28131);
    data.mValueArray.push_back(-1828);
    data.mValueArray.push_back(1992921);
    data.mStructArray.push_back({ 1292 });
    data.mStructArray.push_back({ -1292 });

    DummyStruct output;
    JsonStream ts(Stream::TEXT, &expected, Stream::SM_READ);
    if(ts.GetMode() == Stream::SM_READ)
    {
        ts << output;
        ts.Close();
    }

    TEST(output == data);
}

REGISTER_TEST(JsonStream_DynamicTypeTest, "Core.IO")
{
    TestDynamicStreamDataAPtr a(LFNew<TestDynamicStreamDataA>()); a->SetType(typeof(TestDynamicStreamDataA));
    TestDynamicStreamDataBPtr b(LFNew<TestDynamicStreamDataB>()); b->SetType(typeof(TestDynamicStreamDataB));
    TestDynamicStreamDataCPtr c(LFNew<TestDynamicStreamDataC>()); c->SetType(typeof(TestDynamicStreamDataC));

    a->mValueString = "This is a string";
    a->mValueUInt = 300;
    a->mValueInt = -1002;

    b->mValueString = 9390;
    b->mValueInt = 2002;

    c->mBar = "also a string";
    c->mFoo = 3003;

    TestDynamicStreamDataType write;
    write.Add(a);
    write.Add(b);
    write.Add(c);

    String text;
    JsonStream ts(Stream::TEXT, &text, Stream::SM_PRETTY_WRITE);
    ts.BeginObject("StreamData", "Native");
    write.Serialize(ts);
    ts.EndObject();
    ts.Close();

    TestDynamicStreamDataType read;

    ts.Open(Stream::TEXT, &text, Stream::SM_READ);
    ts.BeginObject("StreamData", "Native");
    read.Serialize(ts);
    ts.EndObject();
    ts.Close();

    TEST(write.mObjects.size() == read.mObjects.size());


}

} // namespace lf
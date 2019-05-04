#include "Core/Test/Test.h"
#include "Core/IO/TextStream.h"

#include "Core/Utility/Log.h"
#include "Core/Utility/Utility.h"

namespace lf {


void TestStrCmp(const String& a, const String& b)
{
    if (a.Size() != b.Size())
    {
        gTestLog.Error(LogMessage("TestStrCmp: a.Size != b.Size {") << a << "!=" << b << "}");
    }
    for (SizeT i = 0; i < Min(a.Size(), b.Size()); ++i)
    {
        if (a[i] != b[i])
        {
            gTestLog.Error(LogMessage("TestStrCmp: a[i] != b[i] where i=") << i);
            return;
        }
    }
}

struct DummyInnerStruct
{
    void Serialize(Stream& s)
    {
        SERIALIZE(s, mSimpleValue, "");
    }
    bool operator==(const DummyInnerStruct& other) const
    {
        return mSimpleValue == other.mSimpleValue;
    }
    bool operator!=(const DummyInnerStruct& other) const
    {
        return mSimpleValue != other.mSimpleValue;
    }
    Int32 mSimpleValue;
};
Stream& operator<<(Stream& s, DummyInnerStruct& self)
{
    self.Serialize(s);
    return s;
}


struct DummyStruct
{
    void Serialize(Stream& s)
    {
        SERIALIZE_STRUCT(s, mStruct, "");
        SERIALIZE_STRUCT_ARRAY(s, mStructArray, "");
        SERIALIZE_ARRAY(s, mValueArray, "");
        SERIALIZE(s, mValue, "");
    }

    bool operator==(const DummyStruct& other) const
    {
        return mStruct == other.mStruct && mStructArray == other.mStructArray && mValueArray == other.mValueArray && mValue == other.mValue;
    }

    DummyInnerStruct            mStruct;
    TArray<DummyInnerStruct>    mStructArray;
    TArray<Int32>               mValueArray;
    Int32                       mValue;
};
Stream& operator<<(Stream& s, DummyStruct& self)
{
    self.Serialize(s);
    return s;
}

REGISTER_TEST(TextStream_EmptyObjectTest)
{
    String expected = "$TestName=TestSuper\n{\n}\n";
    String output;

    TextStream ts;
    ts.Open(Stream::TEXT, &output, Stream::SM_WRITE);
    ts.BeginObject("TestName", "TestSuper");
    ts.EndObject();
    ts.Close();
    TEST(output == expected);
}
REGISTER_TEST(TextStream_MultiEmptyObjectTest)
{
    String expected = "$TestName=TestSuper\n{\n}\n$TestObject=TestSuper\n{\n}\n";
    String output;

    TextStream ts;
    ts.Open(Stream::TEXT, &output, Stream::SM_WRITE);
    ts.BeginObject("TestName", "TestSuper");
    ts.EndObject();
    ts.BeginObject("TestObject", "TestSuper");
    ts.EndObject();
    ts.Close();
    TEST(output == expected);
}

REGISTER_TEST(TextStream_PropertyWriteTest)
{
    String expected("$TestName=TestSuper\n");
    expected +=
    String( "{\n") +
            "    u8val=72\n" +
            "    u16val=21717\n" +
            "    u32val=372282\n" +
            "    u64val=123812347281910\n" +
            "    s8val=-120\n" +
            "    s16val=-23190\n" +
            "    s32val=-8392920\n" + 
            "    s64val=-1283838299291\n" +
            "}\n";

    UInt8  u8val  = 72;
    UInt16 u16val = 21717;
    UInt32 u32val = 372282;
    UInt64 u64val = 123812347281910;
    Int8   s8val  = -120;
    Int16  s16val = -23190;
    Int32  s32val = -8392920;
    Int64  s64val = -1283838299291;

    String output;
    TextStream ts;
    ts.Open(Stream::TEXT, &output, Stream::SM_WRITE);
    ts.BeginObject("TestName", "TestSuper");
    SERIALIZE(ts, u8val, "");
    SERIALIZE(ts, u16val, "");
    SERIALIZE(ts, u32val, "");
    SERIALIZE(ts, u64val, "");
    SERIALIZE(ts, s8val, "");
    SERIALIZE(ts, s16val, "");
    SERIALIZE(ts, s32val, "");
    SERIALIZE(ts, s64val, "");
    ts.EndObject();
    ts.Close();

    TEST(output == expected);
}

REGISTER_TEST(TextStream_PropertyReadTest)
{
    String expected("$TestName=TestSuper\n");
    expected +=
        String("{\n") +
        "    u8val=72\n" +
        "    u16val=21717\n" +
        "    u32val=372282\n" +
        "    u64val=123812347281910\n" +
        "    s8val=-120\n" +
        "    s16val=-23190\n" +
        "    s32val=-8392920\n" +
        "    s64val=-1283838299291\n" +
        "}\n";

    UInt8  u8val  = 0;
    UInt16 u16val = 0;
    UInt32 u32val = 0;
    UInt64 u64val = 0;
    Int8   s8val  = 0;
    Int16  s16val = 0;
    Int32  s32val = 0;
    Int64  s64val = 0;

    TextStream ts;
    ts.Open(Stream::TEXT, &expected, Stream::SM_READ);
    ts.BeginObject("TestName", "TestSuper");
    SERIALIZE(ts, u8val, "");
    SERIALIZE(ts, u16val, "");
    SERIALIZE(ts, u32val, "");
    SERIALIZE(ts, u64val, "");
    SERIALIZE(ts, s8val, "");
    SERIALIZE(ts, s16val, "");
    SERIALIZE(ts, s32val, "");
    SERIALIZE(ts, s64val, "");
    ts.EndObject();
    ts.Close();

    TEST(u8val == 72);
    TEST(u16val == 21717);
    TEST(u32val == 372282);
    TEST(u64val == 123812347281910);
    TEST(s8val == -120);
    TEST(s16val == -23190);
    TEST(s32val == -8392920);
    TEST(s64val == -1283838299291);

}

REGISTER_TEST(TextStream_ComplexWriteTest)
{
    String expected = String("$DummyStruct=native_struct\n") +
        "{\n" +
        "    Struct={\n" +
        "        SimpleValue=173829\n" +
        "    }\n" +
        "    StructArray=[\n" +
        "        {\n" +
        "            SimpleValue=1292\n" +
        "        }\n" +
        "        {\n" +
        "            SimpleValue=-1292\n" +
        "        }\n" +
        "    ]\n" +
        "    ValueArray=[\n" +
        "        28131\n" +
        "        -1828\n" +
        "        1992921\n" +
        "    ]\n" +
        "    Value=1337\n" +
        "}\n";


    DummyStruct data;
    data.mValue = 1337;
    data.mStruct.mSimpleValue = 173829;
    data.mValueArray.Add(28131);
    data.mValueArray.Add(-1828);
    data.mValueArray.Add(1992921);
    data.mStructArray.Add({ 1292 });
    data.mStructArray.Add({ -1292});

    String output;
    TextStream ts;
    ts.Open(Stream::TEXT, &output, Stream::SM_WRITE);
    ts.BeginObject("DummyStruct", "native_struct");
    ts << data;
    ts.EndObject();
    ts.Close();

    TEST(output == expected);
}

REGISTER_TEST(TextStream_ComplexReadTest)
{
    String expected = String("$DummyStruct=native_struct\n") +
        "{\n" +
        "    Struct={\n" +
        "        SimpleValue=173829\n" +
        "    }\n" +
        "    StructArray=[\n" +
        "        {\n" +
        "            SimpleValue=1292\n" +
        "        }\n" +
        "        {\n" +
        "            SimpleValue=-1292\n" +
        "        }\n" +
        "    ]\n" +
        "    ValueArray=[\n" +
        "        28131\n" +
        "        -1828\n" +
        "        1992921\n" +
        "    ]\n" +
        "    Value=1337\n" +
        "}\n";

    DummyStruct data;
    data.mValue = 1337;
    data.mStruct.mSimpleValue = 173829;
    data.mValueArray.Add(28131);
    data.mValueArray.Add(-1828);
    data.mValueArray.Add(1992921);
    data.mStructArray.Add({ 1292 });
    data.mStructArray.Add({ -1292 });

    DummyStruct output;
    TextStream ts;
    ts.Open(Stream::TEXT, &expected, Stream::SM_READ);
    ts.BeginObject("DummyStruct", "native_struct");
    ts << output;
    ts.EndObject();
    ts.Close();

    TEST(output == data);
}

REGISTER_TEST(TextStream_SerializeString)
{
    String mTag = "Character";
    String mBundle = "GameBase";

    String output;
    TextStream ts;
    ts.Open(Stream::TEXT, &output, Stream::SM_WRITE);
    ts.BeginObject("0", "0");
    SERIALIZE(ts, mTag, "");
    SERIALIZE(ts, mBundle, "");
    ts.EndObject();
    ts.Close();

    mTag.Clear();
    mBundle.Clear();
    ts.Open(Stream::TEXT, &output, Stream::SM_READ);
    ts.BeginObject("0", "0");
    SERIALIZE(ts, mTag, "");
    SERIALIZE(ts, mBundle, "");
    ts.EndObject();
    ts.Close();
}

REGISTER_TEST(TextStreamTest)
{
    TestConfig config = TestFramework::GetConfig();
    TestFramework::ExecuteTest("TextStream_EmptyObjectTest", config);
    TestFramework::ExecuteTest("TextStream_MultiEmptyObjectTest", config);
    TestFramework::ExecuteTest("TextStream_PropertyWriteTest", config);
    TestFramework::ExecuteTest("TextStream_PropertyReadTest", config);
    TestFramework::ExecuteTest("TextStream_ComplexWriteTest", config);
    TestFramework::ExecuteTest("TextStream_ComplexReadTest", config);
    TestFramework::ExecuteTest("TextStream_SerializeString", config);
    TestFramework::TestReset();
}

}


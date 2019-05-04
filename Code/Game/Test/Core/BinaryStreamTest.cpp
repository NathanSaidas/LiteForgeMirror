#include "Core/Test/Test.h"
#include "Core/IO/BinaryStream.h"
#include "Core/Utility/Log.h"

namespace lf {

String EncodeBytes(ByteT* bytes, SizeT length, SizeT splitLine = 8)
{
    String encoded;
    for (SizeT i = 0; i < length; ++i)
    {
        if (splitLine != 0)
        {
            if ((i != 0 && (i % splitLine) == 0))
            {
                encoded.Append('\n');
            }
        }
        ToHexStringAppend(static_cast<Int32>(bytes[i]), encoded);
        encoded.Append(' ');
    }
    return encoded;
}

TArray<ByteT> DecodeBytes(String string)
{
    TArray<String> hexTokens;
    string.Replace("\n", "");
    StrSplit(string, ' ', hexTokens);

    TArray<ByteT> hexBytes;
    for (SizeT i = 0; i < hexTokens.Size(); ++i)
    {
        if (hexTokens[i][0] == '\n')
        {
            continue;
        }
        ByteT left = HexToByte(hexTokens[i][0]);
        ByteT right = HexToByte(hexTokens[i][1]);
        ByteT byte = (left << 4) | right;
        hexBytes.Add(byte);
    }
    return hexBytes;
}

REGISTER_TEST(BinaryStream_EmptyObjectTest)
{
    auto expected = DecodeBytes("54 65 73 74 53 75 70 65 72 54 65 73 74 4E 61 6D 65 00 00 00 00 00 00 00 00 09 00 00 00 08 00 00 00 01 00 00 00");

    MemoryBuffer buffer;
    BinaryStream bs;
    bs.Open(Stream::MEMORY, &buffer, Stream::SM_WRITE);
    bs.BeginObject("TestName", "TestSuper");
    bs.EndObject();
    bs.Close();

    // gTestLog.Info(LogMessage("Generated:\n") << EncodeBytes(reinterpret_cast<ByteT*>(buffer.GetData()), buffer.GetSize(), 0));

    TEST_CRITICAL(buffer.GetSize() == expected.Size());
    TEST(memcmp(buffer.GetData(), expected.GetData(), expected.Size()) == 0);
}

REGISTER_TEST(BinaryStream_MultiEmptyObjectTest)
{
    auto expected = DecodeBytes("54 65 73 74 53 75 70 65 72 54 65 73 74 4E 61 6D 65 00 00 00 00 00 00 00 00 09 00 00 00 08 00 00 00 54 65 73 74 53 75 70 65 72 54 65 73 74 4F 62 6A 65 63 74 00 00 00 00 00 00 00 00 09 00 00 00 0A 00 00 00 02 00 00 00");


    MemoryBuffer buffer;
    BinaryStream bs;
    bs.Open(Stream::MEMORY, &buffer, Stream::SM_WRITE);
    bs.BeginObject("TestName", "TestSuper");
    bs.EndObject();
    bs.BeginObject("TestObject", "TestSuper");
    bs.EndObject();
    bs.Close();

    // gTestLog.Info(LogMessage("Generated:\n") << EncodeBytes(reinterpret_cast<ByteT*>(buffer.GetData()), buffer.GetSize(), 0));

    TEST_CRITICAL(buffer.GetSize() == expected.Size());
    TEST(memcmp(buffer.GetData(), expected.GetData(), expected.Size()) == 0);
}

REGISTER_TEST(BinaryStream_PropertyWriteTest)
{
    auto expected = DecodeBytes(
        "48 D5 54 3A AE 05 00 F6 81 FA 4E 9B 70 00 00 88 6A A5 28 EF 7F FF 65 33 46 15 D5 FE FF FF 54 65 73 74 53 75 70 65 72 54 "
        "65 73 74 4E 61 6D 65 1E 00 00 00 00 00 00 00 09 00 00 00 08 00 00 00 01 00 00 00"
    );

    UInt8  u8val = 72;
    UInt16 u16val = 21717;
    UInt32 u32val = 372282;
    UInt64 u64val = 123812347281910;
    Int8   s8val = -120;
    Int16  s16val = -23190;
    Int32  s32val = -8392920;
    Int64  s64val = -1283838299291;

    MemoryBuffer buffer;
    BinaryStream bs;
    bs.Open(Stream::MEMORY, &buffer, Stream::SM_WRITE);
    bs.BeginObject("TestName", "TestSuper");
    SERIALIZE(bs, u8val, "");
    SERIALIZE(bs, u16val, "");
    SERIALIZE(bs, u32val, "");
    SERIALIZE(bs, u64val, "");
    SERIALIZE(bs, s8val, "");
    SERIALIZE(bs, s16val, "");
    SERIALIZE(bs, s32val, "");
    SERIALIZE(bs, s64val, "");
    bs.EndObject();
    bs.Close();

    TEST_CRITICAL(buffer.GetSize() == expected.Size());
    TEST(memcmp(buffer.GetData(), expected.GetData(), expected.Size()) == 0);
}

REGISTER_TEST(BinaryStream_PropertyReadTest)
{
    auto expected = DecodeBytes(
        "48 D5 54 3A AE 05 00 F6 81 FA 4E 9B 70 00 00 88 6A A5 28 EF 7F FF 65 33 46 15 D5 FE FF FF 54 65 73 74 53 75 70 65 72 54 "
        "65 73 74 4E 61 6D 65 1E 00 00 00 00 00 00 00 09 00 00 00 08 00 00 00 01 00 00 00"
    );

    UInt8  u8val = 0;
    UInt16 u16val = 0;
    UInt32 u32val = 0;
    UInt64 u64val = 0;
    Int8   s8val = 0;
    Int16  s16val = 0;
    Int32  s32val = 0;
    Int64  s64val = 0;

    MemoryBuffer buffer;
    buffer.Allocate(expected.Size(), LF_SIMD_ALIGN);
    memcpy(buffer.GetData(), expected.GetData(), expected.Size());
    buffer.SetSize(expected.Size());

    BinaryStream bs;
    bs.Open(Stream::MEMORY, &buffer, Stream::SM_READ);
    bs.BeginObject("TestName", "TestSuper");
    SERIALIZE(bs, u8val, "");
    SERIALIZE(bs, u16val, "");
    SERIALIZE(bs, u32val, "");
    SERIALIZE(bs, u64val, "");
    SERIALIZE(bs, s8val, "");
    SERIALIZE(bs, s16val, "");
    SERIALIZE(bs, s32val, "");
    SERIALIZE(bs, s64val, "");
    bs.EndObject();
    bs.Close();

    TEST(u8val == 72);
    TEST(u16val == 21717);
    TEST(u32val == 372282);
    TEST(u64val == 123812347281910);
    TEST(s8val == -120);
    TEST(s16val == -23190);
    TEST(s32val == -8392920);
    TEST(s64val == -1283838299291);
}

REGISTER_TEST(BinaryStreamTest)
{
    TestConfig config = TestFramework::GetConfig();
    TestFramework::ExecuteTest("BinaryStream_EmptyObjectTest", config);
    TestFramework::ExecuteTest("BinaryStream_MultiEmptyObjectTest", config);
    TestFramework::ExecuteTest("BinaryStream_PropertyWriteTest", config);
    TestFramework::ExecuteTest("BinaryStream_PropertyReadTest", config);
    TestFramework::TestReset();

    // MemoryBuffer buffer;
    // BinaryStream bs;
    // bs.Open(Stream::MEMORY, &buffer, Stream::SM_WRITE);
    // bs.BeginObject("TestName", "TestSuper");
    // bs.EndObject();
    // bs.Close();
    // 
    // String encoded = EncodeBytes(reinterpret_cast<ByteT*>(buffer.GetData()), buffer.GetSize());
    // TArray<ByteT> hexBytes = DecodeBytes(encoded);
    // 
    // TEST_CRITICAL(hexBytes.Size() == buffer.GetSize());
    // TEST(memcmp(hexBytes.GetData(), buffer.GetData(), hexBytes.Size()) == 0);

}

}
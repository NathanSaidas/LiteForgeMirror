#include "SStreamTest.h"
#include "Core/String/SStream.h"
#include "Core/String/StringCommon.h"
#include <Windows.h>

namespace lf {
REGISTER_TEST(SStreamTest)
{
    SStream ss;
    ss << "Hello Kris\n";
    ss << StreamFillLeft(16);
    ss << "Hello Kris\n";
    ss << StreamFillRight(16);
    ss << "Hello Kris\n";
    ss << StreamBoolAlpha(false);
    auto state = ss.Push();

    ss << "Lets build a table\n";
    ss << "+" << StreamFillRight(8) << StreamFillChar('-') << "+" << "+" << StreamFillLeft() << StreamFillChar(' ') << "\n";
    ss << "|" << StreamFillRight(8) << "|" << "|" << StreamFillLeft() << "\n";
    ss << "|" << StreamFillRight(8) << "|" << "|" << StreamFillLeft() << "\n";
    ss << "|" << StreamFillRight(8) << "|" << "|" << StreamFillLeft() << "\n";
    ss << "|" << StreamFillRight(8) << "|" << "|" << StreamFillLeft() << "\n";
    ss << "|" << StreamFillRight(8) << "|" << "|" << StreamFillLeft() << "\n";
    ss << "|" << StreamFillRight(8) << "|" << "|" << StreamFillLeft() << "\n";

    ss.Pop(state);
    ss << "Using previous table state=" << true << "\n";

    ss.Push();
    ss << "Alright let's write default again.. But encode a hex character!" << ToHexString(0xBADF00D) << "\n";




    
    OutputDebugString(ss.CStr());
    LF_DEBUG_BREAK;
}

} // namespace lf
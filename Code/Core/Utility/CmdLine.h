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
#ifndef LF_CORE_CMD_LINE_H
#define LF_CORE_CMD_LINE_H

// #include "core/CoreDef.h"

#include "Core/Reflection/Type.h"
#include "Core/String/String.h"
#include "Core/Utility/Array.h"

namespace lf
{
// CmdLine format
// - denotes start of command
// - <arg name>/<sub option name>=<sub option value>
// CmdArgs can only be specified once! eg -test -reflection -test is not allowed. (the second -test won't appear).

// -reflection /silence=log
// -reflection /silence=warning

// -test /execute=all
// -test /silence_all
// -test /conditions=1
// -test /assert_failure

// todo: Seperate CmdLine into CmdLine (the static interface) and CmdLineParser (the object that parses a string)


struct LF_CORE_API CmdSubOption
{
    String option;
    String value;
};

struct LF_CORE_API CmdArg
{
    CmdArg() :
        name(),
        subOptions()
    {}
    ~CmdArg()
    {
        LF_DEBUG_BREAK;
    }
    String name;
    TArray<CmdSubOption> subOptions;
};

// **********************************
// A helper class to parse command line arguments, you can however create a seperate instance 
// and parse a string if required. 
// 
// Format:
// -Arg /Option=Value
// 
// **********************************
class LF_CORE_API CmdLine
{
public:
    CmdLine() :
        mArgs(),
        mCmdString()
    {
    }
    ~CmdLine()
    {
        LF_DEBUG_BREAK;
    }
    static void ParseCmdLine(const String& str);
    static const String& GetCmdString();
    static bool HasArg(const String& arg);
    static bool HasArgOption(const String& arg, const String& option);
    static bool HasArgOption(const String& arg, const String& option, Int32 value);
    static bool HasArgOption(const String& arg, const String& option, Float32 value);
    static bool HasArgOption(const String& arg, const String& option, const String& value);

    static bool GetArgOption(const String& arg, const String& option, Int32& out);
    static bool GetArgOption(const String& arg, const String& option, Float32& out);
    static bool GetArgOption(const String& arg, const String& option, String& out);
    static void Release();

    void InternalParseCmdLine(const String& str);
    bool InternalHasArg(const String& arg);
    bool InternalHasArgOption(const String& arg, const String& option);
    bool InternalHasArgOption(const String& arg, const String& option, Int32 value);
    bool InternalHasArgOption(const String& arg, const String& option, Float32 value);
    bool InternalHasArgOption(const String& arg, const String& option, const String& value);

    bool InternalGetArgOption(const String& arg, const String& option, Int32& out);
    bool InternalGetArgOption(const String& arg, const String& option, Float32& out);
    bool InternalGetArgOption(const String& arg, const String& option, String& out);

    void InternalRelease();
private:
    /** Creates and returns the singleton instance */
    static CmdLine& GetInstance();

    TArray<CmdArg> mArgs;
    String mCmdString;
};

} // namespace lf


#endif // LF_CORE_CMD_LINE_H
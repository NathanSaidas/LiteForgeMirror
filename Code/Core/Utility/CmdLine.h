// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
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
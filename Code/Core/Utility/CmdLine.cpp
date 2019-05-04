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
#include "CmdLine.h"

#include "Core/String/StringCommon.h"
#include "Core/Math/MathFunctions.h"

#include <algorithm>

namespace lf
{
static CmdLine sCmdLine;
//-silence/test=all/test=reflection-ignore/test=reflection
//-test /execute=all /conditions=1 /silence_all /assert_failure

// Static.

void CmdLine::ParseCmdLine(const String& str)
{
    GetInstance().InternalParseCmdLine(str);
}

const String& CmdLine::GetCmdString()
{
    return GetInstance().mCmdString;
}

bool CmdLine::HasArg(const String& arg)
{
    return GetInstance().InternalHasArg(arg);
}
bool CmdLine::HasArgOption(const String& arg, const String& option)
{
    return GetInstance().InternalHasArgOption(arg, option);
}
bool CmdLine::HasArgOption(const String& arg, const String& option, Int32 value)
{
    return GetInstance().InternalHasArgOption(arg, option, value);
}
bool CmdLine::HasArgOption(const String& arg, const String& option, Float32 value)
{
    return GetInstance().InternalHasArgOption(arg, option, value);
}
bool CmdLine::HasArgOption(const String& arg, const String& option, const String& value)
{
    return GetInstance().InternalHasArgOption(arg, option, value);
}

bool CmdLine::GetArgOption(const String& arg, const String& option, Int32& out)
{
    return GetInstance().InternalGetArgOption(arg, option, out);
}
bool CmdLine::GetArgOption(const String& arg, const String& option, Float32& out)
{
    return GetInstance().InternalGetArgOption(arg, option, out);
}
bool CmdLine::GetArgOption(const String& arg, const String& option, String& out)
{
    return GetInstance().InternalGetArgOption(arg, option, out);
}

void CmdLine::Release()
{
    GetInstance().InternalRelease();
}

// Cmd

void CmdLine::InternalParseCmdLine(const String& argString)
{
    mCmdString = argString;
    mArgs.Clear();

    const Int32 PARSE_DEFAULT = 0;
    const Int32 PARSE_ARG = 1;
    const Int32 PARSE_SUBOPTION = 2;
    const Int32 PARSE_SUBOPTION_VALUE = 3;

    const String::value_type START_ARG = '-';
    const String::value_type START_SUBOPTION = '/';
    const String::value_type START_SUBOPTION_VALUE = '=';

    SizeT breakIndex = 0;
    Int32 mode = 0;
    // commit 
    CmdArg arg;
    CmdSubOption op;

    String stripped = StrStripWhitespace(argString, true);

    bool parsingQuotes = false;
    bool removedQuotes = false;
    for (SizeT i = 0; i < argString.Size(); ++i)
    {
        String::value_type current = argString[i];

        if (mode == PARSE_SUBOPTION_VALUE)
        {
            if (current == '"')
            {
                if (parsingQuotes)
                {
                    removedQuotes = true;
                }
                parsingQuotes = !parsingQuotes;
            }
        }

        if (parsingQuotes)
        {
            continue;
        }

        if (current == START_ARG)
        {
            if (mode == PARSE_SUBOPTION)
            {
                String sub;
                argString.SubString(breakIndex, i - breakIndex, sub);
                sub = StrStripWhitespace(sub);
                op.option = sub;
                arg.subOptions.Add(op);
                mArgs.Add(arg);
                arg = CmdArg();
                op = CmdSubOption();
            }
            else if (mode == PARSE_SUBOPTION_VALUE)
            {
                String sub;
                argString.SubString(breakIndex, i - breakIndex, sub);
                SizeT subLength = INVALID;
                // first non-whitespace
                for (SizeT j = sub.Size() - 1; Valid(j); --j)
                {
                    if (sub[j] != ' ')
                    {
                        subLength = j + 1;
                        break;
                    }
                }
                sub.SubString(0, subLength, op.value);
                if (removedQuotes)
                {
                    op.value.SubString(1, op.value.Size() - 2, sub);
                    op.value = std::move(sub);
                }
                arg.subOptions.Add(op);
                mArgs.Add(arg);
                arg = CmdArg();
                op = CmdSubOption();
            }
            else if (mode == PARSE_ARG)
            {
                String sub;
                argString.SubString(breakIndex, i - breakIndex, sub);
                arg.name = StrStripWhitespace(sub);
                mArgs.Add(arg);
            }
            breakIndex = i + 1;
            mode = PARSE_ARG;
        }
        else if (current == START_SUBOPTION)
        {
            String sub;

            argString.SubString(breakIndex, i - breakIndex, sub);
            sub = StrStripWhitespace(sub);
            if (mode == PARSE_ARG)
            {
                arg.name = sub;
            }
            else if (mode == PARSE_SUBOPTION_VALUE)
            {
                if (removedQuotes)
                {
                    sub.SubString(1, sub.Size() - 2, op.value);
                    removedQuotes = false;
                }
                else
                {
                    op.value = StrTrimRight(sub);
                }
                arg.subOptions.Add(op);
                op = CmdSubOption();
            }
            else if (mode == PARSE_SUBOPTION)
            {
                op.option = StrStripWhitespace(sub);
                if (!op.option.Empty() || !op.value.Empty())
                {
                    arg.subOptions.Add(op);
                }
                op = CmdSubOption();
            }

            breakIndex = i + 1;
            mode = PARSE_SUBOPTION;
        }
        else if (current == START_SUBOPTION_VALUE)
        {
            String sub;
            argString.SubString(breakIndex, i - breakIndex, sub);
            op.option = sub;
            breakIndex = i + 1;
            mode = PARSE_SUBOPTION_VALUE;
        }
        else if (mode == PARSE_SUBOPTION_VALUE && current == '"')
        {
            String sub;
            argString.SubString(breakIndex, sub);
            sub.SubString(1, (i - breakIndex) - 1, op.value);
            breakIndex = i + 1;
            if (!op.value.Empty() || !op.option.Empty())
            {
                arg.subOptions.Add(op);
            }
        }
        else if (i == argString.Size() - 1)
        {
            String sub;
            argString.SubString(breakIndex, sub);
            if (mode == PARSE_ARG)
            {
                arg.name = StrStripWhitespace(sub);
            }
            else if (mode == PARSE_SUBOPTION)
            {
                op.option = StrStripWhitespace(sub);
            }
            else if (mode == PARSE_SUBOPTION_VALUE)
            {
                if (removedQuotes)
                {
                    sub.SubString(1, sub.Size() - 2, op.value);
                    removedQuotes = false;
                }
                else
                {
                    op.value = StrTrimRight(sub);
                }
            }
            if (!op.value.Empty() || !op.option.Empty())
            {
                arg.subOptions.Add(op);
            }
            mArgs.Add(arg);
        }
    }
}

// ==========
// Predicates
// ==========

struct CmdArgFindByName
{
    CmdArgFindByName(const String& str) : compareStr(str) {}

    bool operator()(const CmdArg& cmdArg) const
    {
        return compareStr == cmdArg.name;
    }
    String compareStr;
};

struct CmdSubOptionFindByOption
{
    CmdSubOptionFindByOption(const String& str) : optionStr(str) {};

    bool operator()(const CmdSubOption& subOption) const
    {
        return optionStr == subOption.option;
    }

    String optionStr;
};

struct CmdSubOptionFindInt
{
    CmdSubOptionFindInt(const String& option, Int32 v) : optionStr(option), value(v) {};

    bool operator()(const CmdSubOption& subOption) const
    {
        if (subOption.option == optionStr && StrIsNumber(subOption.value))
        {
            Int32 temp = ToInt32(subOption.value);
            if (temp == value)
            {
                return true;
            }
        }
        return false;
    }

    String optionStr;
    Int32 value;
};

struct CmdSubOptionFindFloat
{

    CmdSubOptionFindFloat(const String& option, Float32 v) : optionStr(option), value(v) {};

    bool operator()(const CmdSubOption& subOption) const
    {
        if (subOption.option == optionStr && StrIsNumber(subOption.value))
        {
            Float32 temp = ToFloat32(subOption.value);
            if (ApproxEquals(temp, value))
            {
                return true;
            }
        }
        return false;
    }

    String optionStr;
    Float32 value;
};

struct CmdSubOptionFindString
{
    CmdSubOptionFindString(const String& option, const String& v) : optionStr(option), value(v) {};

    bool operator()(const CmdSubOption& subOption) const
    {
        return subOption.option == optionStr && subOption.value == value;
    }

    String optionStr;
    String value;
};

struct CmdSubOptionFindNumber
{
    CmdSubOptionFindNumber(const String& option) : optionStr(option) {};

    bool operator()(const CmdSubOption& subOption) const
    {
        return subOption.option == optionStr && StrIsNumber(subOption.value);
    }

    String optionStr;
};

// Getters / Hases


bool CmdLine::InternalHasArg(const String& arg)
{
    auto it = std::find_if(mArgs.begin(), mArgs.end(), CmdArgFindByName(arg));
    return it != mArgs.end();
}
bool CmdLine::InternalHasArgOption(const String& arg, const String& option)
{
    auto it = std::find_if(mArgs.begin(), mArgs.end(), CmdArgFindByName(arg));
    if (it == mArgs.end())
    {
        return false;
    }
    return std::find_if(it->subOptions.begin(), it->subOptions.end(), CmdSubOptionFindByOption(option)) != it->subOptions.end();
}

bool CmdLine::InternalHasArgOption(const String& arg, const String& option, Int32 value)
{
    auto argIter = std::find_if(mArgs.begin(), mArgs.end(), CmdArgFindByName(arg));
    if (argIter == mArgs.end())
    {
        return false;
    }
    auto optionIter = std::find_if(argIter->subOptions.begin(), argIter->subOptions.end(), CmdSubOptionFindInt(option, value));
    return optionIter != argIter->subOptions.end();

}
bool CmdLine::InternalHasArgOption(const String& arg, const String& option, Float32 value)
{
    auto argIter = std::find_if(mArgs.begin(), mArgs.end(), CmdArgFindByName(arg));
    if (argIter == mArgs.end())
    {
        return false;
    }
    auto optionIter = std::find_if(argIter->subOptions.begin(), argIter->subOptions.end(), CmdSubOptionFindFloat(option, value));
    return optionIter != argIter->subOptions.end();
}
bool CmdLine::InternalHasArgOption(const String& arg, const String& option, const String& value)
{
    auto argIter = std::find_if(mArgs.begin(), mArgs.end(), CmdArgFindByName(arg));
    if (argIter == mArgs.end())
    {
        return false;
    }
    auto optionIter = std::find_if(argIter->subOptions.begin(), argIter->subOptions.end(), CmdSubOptionFindString(option, value));
    return optionIter != argIter->subOptions.end();
}

bool CmdLine::InternalGetArgOption(const String& arg, const String& option, Int32& out)
{
    auto argIter = std::find_if(mArgs.begin(), mArgs.end(), CmdArgFindByName(arg));
    if (argIter == mArgs.end())
    {
        return false;
    }
    auto optionIter = std::find_if(argIter->subOptions.begin(), argIter->subOptions.end(), CmdSubOptionFindNumber(option));
    if (optionIter != argIter->subOptions.end())
    {
        out = ToInt32(optionIter->value);
        return true;
    }
    return false;
}
bool CmdLine::InternalGetArgOption(const String& arg, const String& option, Float32& out)
{
    auto argIter = std::find_if(mArgs.begin(), mArgs.end(), CmdArgFindByName(arg));
    if (argIter == mArgs.end())
    {
        return false;
    }
    auto optionIter = std::find_if(argIter->subOptions.begin(), argIter->subOptions.end(), CmdSubOptionFindNumber(option));
    if (optionIter != argIter->subOptions.end())
    {
        out = ToFloat32(optionIter->value);
        return true;
    }
    return false;
}
bool CmdLine::InternalGetArgOption(const String& arg, const String& option, String& out)
{
    auto argIter = std::find_if(mArgs.begin(), mArgs.end(), CmdArgFindByName(arg));
    if (argIter == mArgs.end())
    {
        return false;
    }
    auto optionIter = std::find_if(argIter->subOptions.begin(), argIter->subOptions.end(), CmdSubOptionFindByOption(option));
    if (optionIter != argIter->subOptions.end())
    {
        out = optionIter->value;
        return true;
    }
    return false;
}

void CmdLine::InternalRelease()
{
    mArgs.Clear();
    mCmdString.Clear();
}

CmdLine& CmdLine::GetInstance()
{
    return sCmdLine;
}

} // namespac lf
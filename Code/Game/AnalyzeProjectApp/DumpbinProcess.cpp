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

#include "DumpbinProcess.h"
#include "Core/Common/Assert.h"
#include "Core/Memory/Memory.h"
#include "Core/String/SStream.h"

#if defined(LF_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace lf {

// todo: Maybe find this dynamically or load based on config file...
const char* VC_DUMPBIN_LOCATION = "C:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\bin\\dumpbin";

struct DumpbinProcessInfo
{
    STARTUPINFO         mStartupInfo;
    PROCESS_INFORMATION mProcessInformation;
};

DumpbinProcess::DumpbinProcess() :
    mProcessInfo(nullptr),
    mInput(),
    mOutput()
{}

DumpbinProcess::~DumpbinProcess()
{
    Close();

    if (mProcessInfo)
    {
        LFDelete(mProcessInfo);
    }
}

void DumpbinProcess::Execute(const String& input, const String& output)
{
    if (mProcessInfo)
    {
        Close();
    }
    else
    {
        mProcessInfo = LFNew<DumpbinProcessInfo>();
    }

    ZeroMemory(&mProcessInfo->mStartupInfo, sizeof(mProcessInfo->mStartupInfo));
    ZeroMemory(&mProcessInfo->mProcessInformation, sizeof(mProcessInfo->mProcessInformation));
    mProcessInfo->mStartupInfo.cb = sizeof(mProcessInfo->mStartupInfo);

    mInput = input;
    mOutput = output;

    SStream cmd;
    cmd << "\"" << VC_DUMPBIN_LOCATION << "\" /symbols \"" << mInput << "\" /OUT:\"" << mOutput << "\"";

    mCommandLine = cmd.Str();

    Assert(TRUE == CreateProcessA
    (
        NULL, // Module Name (unused)
        const_cast<char*>(mCommandLine.CStr()), // Process Name
        NULL, // process attributes
        NULL, // thread attributes
        FALSE, // thread attributes
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &mProcessInfo->mStartupInfo,
        &mProcessInfo->mProcessInformation
    ));
}

void DumpbinProcess::Close()
{
    if (mProcessInfo->mProcessInformation.hProcess == NULL)
    {
        return;
    }

    DWORD exitCode = 0;
    ReportBug(GetExitCodeProcess(mProcessInfo->mProcessInformation.hProcess, &exitCode) == TRUE);
    mReturnCode = exitCode;

    WaitForSingleObject(mProcessInfo->mProcessInformation.hProcess, INFINITE);
    CloseHandle(mProcessInfo->mProcessInformation.hProcess);
    CloseHandle(mProcessInfo->mProcessInformation.hThread);

    ZeroMemory(&mProcessInfo->mStartupInfo, sizeof(mProcessInfo->mStartupInfo));
    ZeroMemory(&mProcessInfo->mProcessInformation, sizeof(mProcessInfo->mProcessInformation));
}

bool DumpbinProcess::IsRunning() const
{
    return mProcessInfo && mProcessInfo->mProcessInformation.hProcess != NULL;
}

} // namespace lf
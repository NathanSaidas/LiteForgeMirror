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
#ifndef LF_GAME_DUMPBIN_PROCESS_H
#define LF_GAME_DUMPBIN_PROCESS_H

#include "Core/String/String.h"

namespace lf {

struct DumpbinProcessInfo;

class DumpbinProcess
{
public:
    DumpbinProcess();
    ~DumpbinProcess();

    void Execute(const String& input, const String& output);
    void Close();

    bool IsRunning() const;
    String GetInputFileName() const { return mInput; }
    String GetOutputFileName() const { return mOutput; }
    int GetReturnCode() const { return mReturnCode; }
private:
    DumpbinProcessInfo* mProcessInfo;
    String mInput;
    String mOutput;
    String mCommandLine;
    int mReturnCode;

};

} // namespace lf

#endif // LF_GAME_DUMPBIN_PROCESS_H
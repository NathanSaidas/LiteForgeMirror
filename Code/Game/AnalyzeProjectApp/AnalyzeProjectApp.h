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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ********************************************************************
#ifndef LF_GAME_ANANLYZE_PROJECT_APP_H
#define LF_GAME_ANANLYZE_PROJECT_APP_H

#include "Engine/App/Application.h"

namespace lf {

class AnalyzeProjectApp : public Application
{
    DECLARE_CLASS(AnalyzeProjectApp, Application);
public:
    void OnStart() override;

private:
    struct ObjectCodeCSVRow
    {
        String mFileName;
        SizeT mNumDependencies;
        SizeT mNumDependenciesRecursive;
        SizeT mSize;
        SizeT mNumUndefined;
        SizeT mNumStatic;
        SizeT mNumExternal;
        SizeT mNumCycles;
    };
    struct SourceCodeCSVRow
    {
        String mFileName;
        SizeT mNumDependencies;
        SizeT mNumBadIncludes;
        SizeT mSize;
        SizeT mHasCopyrightNotice;
    };

    struct HeaderCSVRow
    {
        String mFileName;
        SizeT mNumDependencies;
        SizeT mNumDependents;
        SizeT mNumBadIncludes;
        SizeT mSize;
        SizeT mHasCopyrightNotice;
        SizeT mNumCycles;
    };

    String GetTempDirectory() const;

    void AnalyzeOBJ(const String& objDirectory, bool async, TVector<ObjectCodeCSVRow>& outRows, String& outReport, String& outCycles);
    void AnalyzeSource(const String& sourceDirectory, bool async, TVector<SourceCodeCSVRow>& csvRows, TVector<HeaderCSVRow>& headerRows, String& outReport, String& outCycles);
};

} // namespace lf

#endif // LF_GAME_ANANLYZE_PROJECT_APP_H
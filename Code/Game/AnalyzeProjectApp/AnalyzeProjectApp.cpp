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

#include "AnalyzeProjectApp.h"
#include "Core/String/StringHashTable.h"
#include "Core/IO/EngineConfig.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Platform/File.h"
#include "Core/Utility/CmdLine.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Time.h"
#include "Core/Platform/Thread.h"

#include "Runtime/Async/PromiseImpl.h"

#include "Game/AnalyzeProjectApp/DumpbinProcess.h"

#include <algorithm>
#include <map>
#include <set>

namespace lf {
DEFINE_CLASS(AnalyzeProjectApp) { NO_REFLECTION; }

enum ProjectSymbolDependency
{
    PSD_ABS,
    PSD_DEFINED,
    PSD_UNDEF
};

enum ProjectSymbolVisibility
{
    PSV_STATIC,
    PSV_EXTERNAL,
    PSV_LABEL
};

struct StringHashLess
{
    bool operator()(const StringHashTable::HashedString& a, const StringHashTable::HashedString& b) const
    {
        return a.mString < b.mString;
    }
};

DECLARE_HASHED_CALLBACK(ObjectFileCallback, void);

struct ObjectFile
{
    using GenericPromise = PromiseImpl<ObjectFileCallback, ObjectFileCallback>;

    SizeT  mFileSize;
    String mFilename;
    String mSymbolFilename;
    String mSymbolFileText;

    TVector<String> mUndefined;
    TVector<String> mStatic;
    TVector<String> mExternal;

    TVector<StringHashTable::HashedString> mUndefinedHashed;
    TVector<StringHashTable::HashedString> mStaticHashed;
    TVector<StringHashTable::HashedString> mExternalHashed;

    std::map<const ObjectFile*, TVector<StringHashTable::HashedString>> mDependencies;
    TVector<const ObjectFile*> mRecursiveDependencies;

    PromiseWrapper mPromise;

    TVector<TVector<const ObjectFile*>> mCycles;
};

struct CodeFile
{
    String mName;
    String mFileName;
    StringHashTable::HashedString mFileNameHashed;
    StringHashTable::HashedString mNameHashed;
    SizeT  mFileSize;

    String mText;

    bool mHasCopyrightNotice;

    TVector<String> mIncludes;
    TVector<const CodeFile*> mIndexedIncludes;
    TVector<TVector<const CodeFile*>> mCycles;


    TVector<const CodeFile*> mDependencies;
    TVector<const CodeFile*> mDependents;
    std::map<String, TVector<StringHashTable::HashedString>> mBadIncludes;

    bool  mHeader;
};

struct CodeProject
{
    String mName;
    TVector<CodeFile> mFiles;
};

static void ReadSymbols(ObjectFile& file)
{
    File f;
    Assert(f.Open(file.mSymbolFilename, FF_READ, FILE_OPEN_EXISTING));

    file.mSymbolFileText.Clear();
    file.mSymbolFileText.Resize(static_cast<SizeT>(f.GetSize()));
    Assert(f.Read(const_cast<char*>(file.mSymbolFileText.CStr()), file.mSymbolFileText.Size()) == file.mSymbolFileText.Size());
    file.mFileSize = static_cast<SizeT>(f.GetSize());
}

static bool ParseSymbols(const String& text, TVector<String>& undefinedSymbols, TVector<String>& staticSymbols, TVector<String>& externalSymbols)
{
    const String COFF_SYMBOL_TABLE = "COFF SYMBOL TABLE";
    const SizeT TOKEN_SYMBOL_NUMBER = 0;
    const SizeT TOKEN_SYMBOL_DEPENDENCY = 2; // ABS/SEC(N)/UNDEF
    const SizeT TOKEN_SYMBOL_TYPE = 3;       // type
    const SizeT TOKEN_VISIBILITY_OR_TYPE_EX = 4; // If the notype has () after it's apart of the type and TOKEN_VISIBILITY_FUNCTION should be used.
    const SizeT TOKEN_VISIBILITY_FUNCTION = 5;

    const SizeT TOKEN_SYMBOL = 6;
    const SizeT TOKEN_FUNCTION_SYMBOL = 7;

    SizeT pos = text.Find(COFF_SYMBOL_TABLE);
    if (Invalid(pos))
    {
        return false;
    }

    // Read Until Next Line
    for (SizeT i = pos; i < text.Size(); ++i)
    {
        if (text[i] == '\n')
        {
            pos = i + 1;
            break;
        }
    }


    // Begin Parsing:
    bool skipNextLine = false;
    String buffer;
    buffer.Reserve(1024);
    TVector<String> tokens;
    tokens.reserve(10);

    for (SizeT i = pos; i < text.Size(); )
    {
        buffer.Resize(0);

        // ReadLine:
        for (SizeT k = i; k < text.Size(); ++k)
        {
            if (text[k] == '\n')
            {
                if (text[k - 1] == '\r')
                {
                    text.SubString(i, (k - i) - 1, buffer);
                }
                else
                {
                    text.SubString(i, k - i, buffer);
                }
                i = k + 1;
                break;
            }
        }

        if (skipNextLine)
        {
            skipNextLine = false;
            continue;
        }

        Assert(Invalid(buffer.FindLast('\r')));
        if (buffer.Empty())
        {
            break;
        }
        Assert(Invalid(buffer.Find("String Table Size")));

        tokens.resize(0);
        StrSplit(buffer, ' ', tokens);

        if 
        (
            tokens.empty() || 
            (tokens.size() > TOKEN_SYMBOL_DEPENDENCY && tokens[TOKEN_SYMBOL_DEPENDENCY] == "ABS") || // ignored:
            (tokens.size() >= 2 && tokens[0] == "Relocation" && tokens[1] == "CRC")
        )
        {
            continue;
        }

        bool isFunction = tokens[TOKEN_VISIBILITY_OR_TYPE_EX] == "()";
        // Skip non-functions
        if (!isFunction)
        {
            String* symbolText = nullptr;
            if (isFunction)
            {
                symbolText = &tokens[TOKEN_FUNCTION_SYMBOL];
            }
            else
            {
                symbolText = &tokens[TOKEN_SYMBOL];
            }

            // data symbols have extra information on the next line.
            ReportBug(!symbolText->Empty());
            if (symbolText->First() == '.')
            {
                skipNextLine = true;
            }
            continue;
        }

        ProjectSymbolDependency dependency = PSD_ABS;
        if (tokens[TOKEN_SYMBOL_DEPENDENCY] == "UNDEF")
        {
            dependency = PSD_UNDEF;
        }
        else if (Valid(tokens[TOKEN_SYMBOL_DEPENDENCY].Find("SECT")))
        {
            dependency = PSD_DEFINED;
        }
        else
        {
            CriticalAssertMsgEx("Unexpected token Symbol", LF_ERROR_INVALID_OPERATION, ERROR_API_GAME);
        }

        ProjectSymbolVisibility vis = PSV_LABEL;
        if (tokens[TOKEN_VISIBILITY_FUNCTION] == "External")
        {
            vis = PSV_EXTERNAL;
        }
        else if (tokens[TOKEN_VISIBILITY_FUNCTION] == "WeakExternal")
        {
            vis = PSV_EXTERNAL;
            skipNextLine = true;
        }
        else if (tokens[TOKEN_VISIBILITY_FUNCTION] == "Static")
        {
            vis = PSV_STATIC;
        }
        else
        {
            CriticalAssertMsgEx("Unexpected token Visibility", LF_ERROR_INVALID_OPERATION, ERROR_API_GAME);
        }

        if (dependency == PSD_UNDEF && vis == PSV_STATIC)
        {
            CriticalAssertMsgEx("Unexpected dependency/visibility combination", LF_ERROR_INVALID_OPERATION, ERROR_API_GAME);
        }

        if (dependency == PSD_UNDEF)
        {
            undefinedSymbols.push_back(tokens[TOKEN_FUNCTION_SYMBOL]);
        }
        else if (vis == PSV_EXTERNAL)
        {
            externalSymbols.push_back(tokens[TOKEN_FUNCTION_SYMBOL]);
        }
        else if (vis == PSV_STATIC)
        {
            staticSymbols.push_back(tokens[TOKEN_FUNCTION_SYMBOL]);
        }
        else
        {
            CriticalAssertMsgEx("Unhandled symbol type.", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        }
    }
    return true;
}

static TVector<ObjectFile> GetOBJFiles(const String& directory, const String& tempDirectory)
{
    gSysLog.Info(LogMessage("Calculating Object Files to analyze..."));

    TVector<String> objFiles;
    FileSystem::GetAllFiles(directory, objFiles);

    for (auto it = objFiles.begin(); it != objFiles.end();)
    {
        if (Invalid(it->FindLast(".obj")))
        {
            it = objFiles.swap_erase(it);
        }
        else
        {
            ++it;
        }
    }

    TVector<ObjectFile> files;
    files.resize(objFiles.size());

    for (SizeT i = 0, size = files.size(); i < size; ++i)
    {

        String name = objFiles[i].SubString(directory.Size());
        name.Replace(".obj", ".symbols.txt");
        name = FileSystem::PathJoin(tempDirectory, name);

        files[i].mFilename = objFiles[i];
        files[i].mSymbolFilename = name;
    }

    return files;
}

static void ExportSymbols(const TVector<ObjectFile>& objFiles, SizeT fileCount, SizeT waitTimeMilliseconds)
{
    TVector<DumpbinProcess> dumpbinProcesses;
    dumpbinProcesses.resize(objFiles.size());

    gSysLog.Info(LogMessage("Exporting ") << objFiles.size() << " Object Files...");

    SizeT activeIndex = 0;
    for (SizeT i = 0; i < objFiles.size(); ++i)
    {
        // DumpbinProcess& proc = processes[i];

        if (i != 0 && (i % fileCount) == 0)
        {
            gSysLog.Info(LogMessage(i) << "/" << objFiles.size() << " Pausing for dumpbin...");
            SleepCallingThread(waitTimeMilliseconds);
            
            // Retire Previous Processes...
            for (SizeT k = activeIndex; k < i; ++k)
            {
                dumpbinProcesses[k].Close();
                ReportBug(dumpbinProcesses[k].GetReturnCode() == 0 || dumpbinProcesses[k].GetReturnCode() == 259);
            }
        }

        // Dumpbin needs a complete path to put the file...
        SizeT dir = objFiles[i].mSymbolFilename.FindLast('\\');
        Assert(Valid(dir));
        String path = objFiles[i].mSymbolFilename.SubString(0, dir);
        FileSystem::PathCreate(path);

        // Execute the process.
        dumpbinProcesses[i].Execute(objFiles[i].mFilename, objFiles[i].mSymbolFilename);
    }

    SleepCallingThread(waitTimeMilliseconds);
    for (DumpbinProcess& proc : dumpbinProcesses)
    {
        if (proc.IsRunning())
        {
            proc.Close();
            ReportBug(proc.GetReturnCode() == 0);
        }
    }
}

static void CalculateDependencies(ObjectFile& file, const TVector<ObjectFile>& objectFiles)
{
    for (auto& hashedString : file.mUndefinedHashed)
    {
        TVector<const ObjectFile*> owners;
        for (const ObjectFile& owner : objectFiles)
        {
            auto iter = std::find_if(owner.mExternalHashed.begin(), owner.mExternalHashed.end(),
                [hashedString](const StringHashTable::HashedString& str)
            {
                return str.mString == hashedString.mString;
            });

            if (iter != owner.mExternalHashed.end())
            {
                owners.push_back(&owner);
            }
        }


        for (const ObjectFile* owner : owners)
        {
            file.mDependencies[owner].push_back(hashedString);
        }
    }
}

static void CalculateDependenciesRecursive
(
    ObjectFile& rootFile, 
    const ObjectFile& current, 
    const TVector<ObjectFile>& objectFiles,
    std::set<const ObjectFile*>& visited,
    TVector<const ObjectFile*>& stack
)
{
    if (!visited.emplace(&current).second)
    {
        return; // already visited.
    }

    for (auto& hashedString : current.mUndefinedHashed)
    {
        for (const ObjectFile& owner : objectFiles)
        {
            auto iter = std::find_if(owner.mExternalHashed.begin(), owner.mExternalHashed.end(),
                [hashedString](const StringHashTable::HashedString& str)
            {
                return str.mString == hashedString.mString;
            });

            if (iter != owner.mExternalHashed.end())
            {
                // cycle detected:
                if (&owner == &rootFile)
                {
                    rootFile.mCycles.push_back(stack);
                    return;
                }

                // already processed.
                if (visited.find(&owner) == visited.end())
                {
                    stack.push_back(&owner);
                    CalculateDependenciesRecursive(rootFile, owner, objectFiles, visited, stack);
                    stack.pop_back();
                }
            }
        }
    }


}

static void CalculateDependenciesRecursive(ObjectFile& file, TVector<ObjectFile>& objFiles)
{
    std::set<const ObjectFile*> visited;
    TVector<const ObjectFile*> stack;

    stack.push_back(&file);
    CalculateDependenciesRecursive(file, file, objFiles, visited, stack);
    stack.pop_back();
    Assert(stack.empty());

    for (auto dependency : visited)
    {
        if (dependency == &file)
        {
            continue;
        }
        file.mRecursiveDependencies.push_back(dependency);
    }
}

static TVector<CodeFile> GetSourceFiles(const String& directory)
{
    TVector<String> sourceFiles;
    FileSystem::GetAllFiles(directory, sourceFiles);
    for (auto it = sourceFiles.begin(); it != sourceFiles.end();)
    {
        SizeT ext = it->FindLast('.');
        SizeT h = it->FindLast(".h");
        SizeT cpp = it->FindLast(".cpp");

        if
        (
            Invalid(ext) || 
            (
                (Invalid(h) || h < ext) &&
                (Invalid(cpp) || cpp < ext)
            )
        )
        {
            if (Valid(it->FindLast(".h.txt")))
            {
                LF_DEBUG_BREAK;
            }

            it = sourceFiles.swap_erase(it);
        }
        else
        {
            ++it;
        }
    }

    TVector<CodeFile> files;
    files.resize(sourceFiles.size());

    for (SizeT i = 0, size = files.size(); i < size; ++i)
    {
        files[i].mFileName = sourceFiles[i];
        files[i].mName = sourceFiles[i].SubString(directory.Size() + 1);
        files[i].mHeader = Valid(sourceFiles[i].FindLast(".h"));
    }

    return files;
}

static void ReadSource(CodeFile& file)
{
    File f;
    Assert(f.Open(file.mFileName, FF_READ, FILE_OPEN_EXISTING));

    if (f.GetSize() == 0)
    {
        file.mFileSize = 0;
        return;
    }

    file.mText.Clear();
    file.mText.Resize(static_cast<SizeT>(f.GetSize()));
    Assert(f.Read(const_cast<char*>(file.mText.CStr()), file.mText.Size()) == file.mText.Size());
    file.mFileSize = static_cast<SizeT>(f.GetSize());
}

static void ParseIncludes(const String& text, const String& relativePrefix, TVector<String>& includes)
{
    const String INCLUDE("#include ");

    String readBuffer;
    readBuffer.Reserve(256);

    String parseBuffer;
    parseBuffer.Reserve(256);

    bool ignoreCommentLine = false;
    SizeT ignoreCommentBlock = 0;

    for (SizeT i = 0, size = text.Size(); i < size; ++i)
    {
        if (text[i] == '/' && i > 0 && text[i-1] == '/')
        {
            ignoreCommentLine = true;
        }

        if (text[i] == '*' && i > 0 && text[i-1] == '/')
        {
            ++ignoreCommentBlock;
        }

        if (text[i] == '/' && i > 0 && text[i-1] == '*')
        {
            Assert(ignoreCommentBlock > 0);
            --ignoreCommentBlock;
        }

        if (text[i] == '\r')
        {
            continue;
        }
        // Read line:
        if (text[i] == '\n')
        {
            if (!ignoreCommentLine && ignoreCommentBlock == 0)
            {
                SizeT includeIndex = readBuffer.Find(INCLUDE);
                if (Valid(includeIndex))
                {
                    readBuffer.SubString(includeIndex + INCLUDE.Size(), parseBuffer);

                    SizeT beginQuote = parseBuffer.Find('\"');
                    SizeT endQuote = INVALID;
                    if (Valid(beginQuote))
                    {
                        parseBuffer.SubString(beginQuote + 1, readBuffer);
                        endQuote = readBuffer.Find('\"');
                    }

                    if (Valid(beginQuote) && Valid(endQuote))
                    {
                        String includePath = FileSystem::PathCorrectPath(readBuffer.SubString(0, endQuote));
                        if (Invalid(includePath.Find('\\')))
                        {
                            includePath = relativePrefix + includePath;
                        }
                        includes.push_back(includePath);
                    }
                }
            }


            readBuffer.Resize(0);
            ignoreCommentLine = false;
            continue;
        }
        readBuffer += text[i];
    }
}

static bool ParseCopyrightNotice(const String& text)
{
    return Valid(text.Find("Copyright (c)"));
}

static void IndexIncludes(CodeFile& file, const TVector<CodeFile>& sourceFiles, const StringHashTable& symbolTable)
{
    for (const String& include : file.mIncludes)
    {
        auto includeHash = symbolTable.Find(include.CStr(), include.Size());
        if (includeHash.Valid())
        {
            auto iter = std::find_if(sourceFiles.begin(), sourceFiles.end(), [includeHash](const CodeFile& cf) { return cf.mNameHashed.mString == includeHash.mString; });
            if (iter != sourceFiles.end())
            {
                file.mIndexedIncludes.push_back(&(*iter));
            }
        }
        else
        {
            file.mBadIncludes[include].push_back(file.mFileNameHashed);
        }
    }
}

static void CalculateDependenciesRecursive
(
    CodeFile& root,
    const CodeFile& current,
    std::set<const CodeFile*>& visited,
    TVector<const CodeFile*>& stack
)
{
    if (!visited.emplace(&current).second)
    {
        return; // already visited
    }

    for (auto& include : current.mIndexedIncludes)
    {
        // cycle detected:
        if (include == &root)
        {
            root.mCycles.push_back(stack);
            return;
        }

        if (visited.find(include) == visited.end())
        {
            stack.push_back(include);
            CalculateDependenciesRecursive(root, *include, visited, stack);
            stack.pop_back();
        }
    }
}

static void CalculateDependenciesRecursive(CodeFile& file)
{
    std::set<const CodeFile*> visited;
    TVector<const CodeFile*> stack;

    stack.push_back(&file);
    CalculateDependenciesRecursive(file, file, visited, stack);
    stack.pop_back();
    Assert(stack.empty());

    for (auto dep : visited)
    {
        if (dep == &file)
        {
            continue;
        }
        file.mDependencies.push_back(dep);
    }
}

String AnalyzeProjectApp::GetTempDirectory() const
{

    String tempDirectory;
    if (GetConfig())
    {
        tempDirectory = FileSystem::PathJoin(GetConfig()->GetTempDirectory(), "AnalyzeProject");
    }
    else
    {
        tempDirectory = FileSystem::PathJoin(FileSystem::PathGetParent(FileSystem::GetWorkingPath()), "Temp\\AnalyzeProject");
    }

    if (!FileSystem::PathExists(tempDirectory) && !FileSystem::PathCreate(tempDirectory))
    {
        return String();
    }
    return tempDirectory;
}

void AnalyzeProjectApp::AnalyzeOBJ(const String& objDirectory, bool async, TVector<ObjectCodeCSVRow>& outRows, String& outReport, String& outCycles)
{
    Int64 clockBegin = 0;
    Int64 clockEnd = 0;

    gSysLog.Info(LogMessage("Analyzing Obj Files"));
    String tempDirectory = FileSystem::PathJoin(GetTempDirectory(), "ObjectFiles");
    if (tempDirectory.Empty())
    {
        gSysLog.Error(LogMessage("Failed to create temp directory ") << tempDirectory);
        return;
    }

    TVector<ObjectFile> objFiles = GetOBJFiles(objDirectory, tempDirectory);
    ExportSymbols(objFiles, 100, 2500);

    gSysLog.Info(LogMessage("Reading and Parsing Symbols..."));
    clockBegin = GetClockTime();
    for (ObjectFile& file : objFiles)
    {
        if (async)
        {
            ObjectFile* filePtr = &file;

            file.mPromise = ObjectFile::GenericPromise([filePtr](Promise* self)
            {
                auto promise = static_cast<ObjectFile::GenericPromise*>(self);

                ReadSymbols(*filePtr);
                if (!ParseSymbols(filePtr->mSymbolFileText, filePtr->mUndefined, filePtr->mStatic, filePtr->mExternal))
                {
                    promise->Reject();
                }
                else
                {
                    promise->Resolve();
                }
            })
            .Execute();
        }
        else
        {
            ReadSymbols(file);
            ParseSymbols(file.mSymbolFileText, file.mUndefined, file.mStatic, file.mExternal);
        }
    }

    if (async)
    {
        Async::WaitAll(objFiles.begin(), objFiles.end(), [](const ObjectFile& file) { return file.mPromise->IsDone(); });
    }
    clockEnd = GetClockTime();
    gSysLog.Info(LogMessage("Elapsed Time=") << ((clockEnd - clockBegin) / static_cast<Float64>(GetClockFrequency())));

    StringHashTable symbolTable;
    auto hash = [&symbolTable](TVector<String>& strings, TVector<StringHashTable::HashedString>& hashedStrings)
    {
        hashedStrings.reserve(strings.size());
        for (const String& str : strings)
        {
            hashedStrings.push_back(symbolTable.Create(str.CStr(), str.Size()));
        }
        strings.clear();
    };

    gSysLog.Info(LogMessage("Hashing symbols..."));

    SizeT before = LFGetBytesAllocated();
    for (ObjectFile& file : objFiles)
    {
        hash(file.mUndefined, file.mUndefinedHashed);
        hash(file.mStatic, file.mStaticHashed);
        hash(file.mExternal, file.mExternalHashed);
    }
    SizeT after = LFGetBytesAllocated();

    gSysLog.Info(LogMessage("Generated ") << symbolTable.Size() << " hashed symbols with " << symbolTable.Collisions() << " collisions and " << (after - before) << " bytes saved.");

    gSysLog.Info(LogMessage("Calculating Dependencies..."));
    clockBegin = GetClockTime();
    for (ObjectFile& file : objFiles)
    {
        if (async)
        {
            ObjectFile* filePtr = &file;
            TVector<ObjectFile>* objectFiles = &objFiles;
            file.mPromise = ObjectFile::GenericPromise([filePtr, objectFiles](Promise* self)
            {
                auto promise = static_cast<ObjectFile::GenericPromise*>(self);

                CalculateDependencies(*filePtr, *objectFiles);
                promise->Resolve();
            })
            .Execute();

        }
        else
        {
            CalculateDependencies(file, objFiles);
        }
        
    }

    if (async)
    {
        Async::WaitAll(objFiles.begin(), objFiles.end(), [](const ObjectFile& file) { return file.mPromise->IsDone(); });
    }
    clockEnd = GetClockTime();
    gSysLog.Info(LogMessage("Elapsed Time=") << ((clockEnd - clockBegin) / static_cast<Float64>(GetClockFrequency())));
    // Calculate Recursive Dependencies

    gSysLog.Info(LogMessage("Calculating Recursive Dependencies and Cycles..."));
    clockBegin = GetClockTime();
    for (ObjectFile& file : objFiles)
    {
        if (async)
        {
            ObjectFile* objectFile = &file;
            TVector<ObjectFile>* objectFiles = &objFiles;
            file.mPromise = ObjectFile::GenericPromise([objectFile, objectFiles](Promise* self)
            {
                auto promise = static_cast<ObjectFile::GenericPromise*>(self);
                CalculateDependenciesRecursive(*objectFile, *objectFiles);
                promise->Resolve();
            })
            .Execute();
        }
        else
        {
            CalculateDependenciesRecursive(file, objFiles);
        }
        
    }

    if (async)
    {
        Async::WaitAll(objFiles.begin(), objFiles.end(), [](const ObjectFile& file) { return file.mPromise->IsDone(); });
    }
    clockEnd = GetClockTime();
    gSysLog.Info(LogMessage("Elapsed Time=") << ((clockEnd - clockBegin) / static_cast<Float64>(GetClockFrequency())));

    outRows.reserve(objFiles.size());

    SStream report;
    SStream cycles;

    report.Reserve(objFiles.size() * 512);
    for (ObjectFile& file : objFiles)
    {
        // Add CSV row
        outRows.push_back(ObjectCodeCSVRow());
        auto& row = outRows.back();
        row.mFileName = file.mFilename.SubString(objDirectory.Size());
        row.mNumDependencies = file.mDependencies.size();
        row.mNumDependenciesRecursive = file.mRecursiveDependencies.size();
        row.mSize = file.mFileSize;
        row.mNumUndefined = file.mUndefinedHashed.size();
        row.mNumStatic = file.mStaticHashed.size();
        row.mNumExternal = file.mExternalHashed.size();
        row.mNumCycles = file.mCycles.size();

        // Write Dependency Report
        if (!file.mRecursiveDependencies.empty() || !file.mDependencies.empty())
        {
            report << file.mFilename << ": Deps=" << row.mNumDependencies << ", Recursive=" << row.mNumDependenciesRecursive << "\n";
        }

        if (!file.mRecursiveDependencies.empty())
        {
            report << "  Recursive Deps:\n";
            for (auto& dep : file.mRecursiveDependencies)
            {
                report << "    " << dep->mFilename << "\n";
            }
        }
        if (!file.mDependencies.empty())
        {
            report << "  Deps:\n";
            for (auto dep : file.mDependencies)
            {
                report << "    " << dep.first->mFilename << ":\n";
                for (auto symbol : dep.second) 
                {
                    report << "      " << symbol.mString << "\n";
                }
            }
        }

        // Write Cycles
        if (!file.mCycles.empty())
        {
            cycles << file.mFilename << ": Cycles=" << row.mNumCycles << "\n";
            for (auto& cycle : file.mCycles)
            {
                cycles << "  " << cycle.front()->mFilename << " <---> " << cycle.back()->mFilename << "\n";
                for (auto& cycleFile : cycle)
                {
                    cycles << "    " << cycleFile->mFilename << "\n";
                }
            }
        }
    }

    outCycles = cycles.Str();
    outReport = report.Str();
}




void AnalyzeProjectApp::AnalyzeSource(const String& sourceDirectory, bool async, TVector<SourceCodeCSVRow>& srcRows, TVector<HeaderCSVRow>& headerRows, String& outReport, String& outCycles)
{
    (async);

    LF_LOG_INFO(gSysLog, FileSystem::PathResolve(sourceDirectory));
    gSysLog.Sync();
    // Get all source/header files...
    auto sourceFiles = GetSourceFiles(sourceDirectory);

    // foreach file parse includes
    for (auto& file : sourceFiles)
    {
        ReadSource(file);

        SizeT dir = file.mName.FindLast('\\');
        String relativeDir;
        if (Valid(dir))
        {
            relativeDir = file.mName.SubString(0, dir + 1);
        }

        ParseIncludes(file.mText, relativeDir,file.mIncludes);
        file.mHasCopyrightNotice = ParseCopyrightNotice(file.mText);
    }

    // foreach file generate include filename hashes.
    StringHashTable symbolTable;
    for (CodeFile& file : sourceFiles)
    {
        file.mFileNameHashed = symbolTable.Create(file.mFileName.CStr(), file.mFileName.Size());
        file.mNameHashed = symbolTable.Create(file.mName.CStr(), file.mName.Size());
    }

    // cache includes
    for (CodeFile& file : sourceFiles)
    {
        IndexIncludes(file, sourceFiles, symbolTable);
    }

    // Calculate recursive dependencies
    for (CodeFile& file : sourceFiles)
    {
        CalculateDependenciesRecursive(file);
    }

    // Calculate dependents.
    for (CodeFile& file : sourceFiles)
    {
        for (CodeFile& other : sourceFiles)
        {
            if (&file == &other)
            {
                continue;
            }

            if (other.mDependencies.end() != std::find(other.mDependencies.begin(), other.mDependencies.end(), &file))
            {
                file.mDependents.push_back(&other);
            }
        }
    }

    SStream report;
    SStream cycles;

    // Export Rows:
    for (CodeFile& file : sourceFiles)
    {
        if (file.mHeader)
        {
            headerRows.push_back(HeaderCSVRow());
            auto& row = headerRows.back();
            row.mFileName = file.mFileName;
            row.mNumDependencies = file.mDependencies.size();
            row.mNumDependents = file.mDependents.size();
            row.mNumBadIncludes = file.mBadIncludes.size();
            row.mSize = file.mFileSize;
            row.mHasCopyrightNotice = file.mHasCopyrightNotice;
            row.mNumCycles = file.mCycles.size();

        }
        else
        {
            srcRows.push_back(SourceCodeCSVRow());
            auto& row = srcRows.back();
            row.mFileName = file.mFileName;
            row.mNumDependencies = file.mDependencies.size();
            row.mNumBadIncludes = file.mBadIncludes.size();
            row.mSize = file.mFileSize;
            row.mHasCopyrightNotice = file.mHasCopyrightNotice;
        }

        if (!file.mDependencies.empty() || !file.mDependents.empty())
        {
            report << file.mFileName << ": Dependencies=" << file.mDependencies.size() << ", Dependents=" << file.mDependents.size() << "\n";
        }

        if (!file.mDependencies.empty())
        {
            report << "  Dependencies:\n";
            for (auto& dep : file.mDependencies)
            {
                report << "    " << dep->mFileName << "\n";
            }
        }

        if (!file.mDependents.empty())
        {
            report << "  Dependents:\n";
            for (auto& dep : file.mDependents)
            {
                report << "    " << dep->mFileName << "\n";
            }
        }

        if (!file.mCycles.empty() || !file.mHasCopyrightNotice)
        {
            cycles << file.mFileName << ": Cycles=" << file.mCycles.size() << ", Copyright=" << file.mHasCopyrightNotice << "\n";
            if (!file.mCycles.empty())
            {
                cycles << "  Cycles:\n";
                for (auto& cycle : file.mCycles)
                {
                    for (auto& cycleFile : cycle)
                    {
                        cycles << "    " << cycleFile->mFileName << "\n";
                    }
                }
            }
        }
    }

    outReport = report.Str();
    outCycles = cycles.Str();
}

void AnalyzeProjectApp::OnStart()
{
    FileSystem::PathDelete(GetTempDirectory());
    FileSystem::PathCreate(FileSystem::PathJoin(GetTempDirectory(), "ObjectFiles"));
    
    // AnalyzeProject
    // -AnalyzeProject 
    //  /OBJ=<path to obj file root directory>
    //  /SOURCE=<path to source root directory>


    

    String objDirectory;
    if (!CmdLine::GetArgOption("AnalyzeProject", "OBJ", objDirectory))
    {
        gSysLog.Warning(LogMessage("AnalyzeProject requires the argument 'OBJ' to be used. 'OBJ' is the path to the root directory of all the .obj files to analyze."));
    }
    String sourceDirectory;
    if (!CmdLine::GetArgOption("AnalyzeProject", "SOURCE", sourceDirectory))
    {
        gSysLog.Warning(LogMessage("AnalyzeProject requires the argument 'SOURCE' to be used. 'SOURCE' is the path to the root directory of all the source code (.cpp/.h)"));
    }

    // Output Header Cycles: (must fix)
    //
    // Output Link-Time Cycles: (must fix)
    // 
    // Output Object Code Information: (filename...)
    // .csv [ Object File, # Dependencies, # Dependencies (recursive), Size, #Undefined, #Static, #External )
    // .txt Dependency List
    // Output Header Information: 
    // .csv [ Source File, # Header Dependencies, Size, Has Copyright Notice, # Bad Includes ]
    // .csv [ Header File, # Header Dependencies, # Header Dependents, Size, Has Copyright Notice, # Bad Includes ]
    // 

    SizeT missingCopyrightNotices = 0;
    SizeT headerCycles = 0;
    SizeT objectFileCycles = 0;
    SizeT badIncludes = 0;

    SizeT numHeaders = 0;
    SizeT numSources = 0;
    SizeT numObjects = 0;

    TVector<String> fixCopyright;

    if (!sourceDirectory.Empty())
    {
        gSysLog.Info(LogMessage("Analyzing source files..."));

        TVector<SourceCodeCSVRow> srcRows;
        TVector<HeaderCSVRow> headerRows;
        String outReport;
        String outCycles;

        AnalyzeSource(sourceDirectory, false, srcRows, headerRows, outReport, outCycles);

        SStream csv;
        csv.Reserve(srcRows.size() * 256);

        csv << "File, # Headers, # Bad Includes, Size, Has Copyright Notice\n";
        for (auto& row : srcRows)
        {
            csv << row.mFileName
                << "," << row.mNumDependencies
                << "," << row.mNumBadIncludes
                << "," << row.mSize
                << "," << row.mHasCopyrightNotice << "\n";
        }

        File output;
        if (output.Open(FileSystem::PathJoin(GetTempDirectory(), "Source.csv"), FF_READ | FF_WRITE, FILE_OPEN_ALWAYS))
        {
            output.Write(csv.CStr(), csv.Size());
            output.Close();
        }

        csv.Reserve(srcRows.size() * 256);
        csv.Clear();

        csv << "File, # Headers, # Dependents, # Cycles, # Bad Includes, Size, Has Copyright Notice\n";
        for (auto& row : headerRows)
        {
            csv << row.mFileName
                << "," << row.mNumDependencies
                << "," << row.mNumDependents
                << "," << row.mNumCycles
                << "," << row.mNumBadIncludes
                << "," << row.mSize
                << "," << row.mHasCopyrightNotice << "\n";
        }

        if (output.Open(FileSystem::PathJoin(GetTempDirectory(), "Headers.csv"), FF_READ | FF_WRITE, FILE_OPEN_ALWAYS))
        {
            output.Write(csv.CStr(), csv.Size());
            output.Close();
        }

        if (output.Open(FileSystem::PathJoin(GetTempDirectory(), "Source.txt"), FF_READ | FF_WRITE, FILE_OPEN_ALWAYS))
        {
            SStream ss;
            ss << "Info for source/header files within " << sourceDirectory << "\n";

            output.Write(outReport.CStr(), outReport.Size());
            output.Close();
        }

        if (output.Open(FileSystem::PathJoin(GetTempDirectory(), "SourceCycles.txt"), FF_READ | FF_WRITE, FILE_OPEN_ALWAYS))
        {
            SStream ss;
            ss << "Include Cycles within " << sourceDirectory << "\n";

            output.Write(ss.CStr(), ss.Size());
            output.Write(outCycles.CStr(), outCycles.Size());
            output.Close();
        }

        for (auto& row : headerRows)
        {
            if (!row.mHasCopyrightNotice)
            {
                fixCopyright.push_back(row.mFileName);
                ++missingCopyrightNotices;
            }
            if (Invalid(row.mFileName.Find("SampleCycle")))
            {
                headerCycles += row.mNumCycles;
            }
            badIncludes += row.mNumBadIncludes;
        }

        for (auto& row : srcRows)
        {
            if (!row.mHasCopyrightNotice)
            {
                fixCopyright.push_back(row.mFileName);
                ++missingCopyrightNotices;
            }
            badIncludes += row.mNumBadIncludes;
        }

        numHeaders += headerRows.size();
        numSources += srcRows.size();
    }

    
    if (!objDirectory.Empty())
    {
        gSysLog.Info(LogMessage("Analyzing object files..."));

        TVector<ObjectCodeCSVRow> objRows;
        String objReport;
        String objCycles;

        AnalyzeOBJ(objDirectory, true, objRows, objReport, objCycles);

        SStream objCSV;
        objCSV.Reserve(objRows.size() * 256);

        objCSV << "File,# Dependencies,# Dependencies Recursive, File Size, # Undefined, # Static, # External, # Cycles\n";

        for (auto& row : objRows)
        {
            objCSV << row.mFileName
                << "," << row.mNumDependencies
                << "," << row.mNumDependenciesRecursive
                << "," << row.mSize
                << "," << row.mNumUndefined
                << "," << row.mNumStatic
                << "," << row.mNumExternal
                << "," << row.mNumCycles << "\n";
        }

        File output;
        if (output.Open(FileSystem::PathJoin(GetTempDirectory(), "ObjectFile.csv"), FF_READ | FF_WRITE, FILE_OPEN_ALWAYS))
        {
            output.Write(objCSV.CStr(), objCSV.Size());
            output.Close();
        }
        
        if (output.Open(FileSystem::PathJoin(GetTempDirectory(), "ObjectFile.txt"), FF_READ | FF_WRITE, FILE_OPEN_ALWAYS))
        {
            SStream ss;
            ss << "Object Info for object files within " << objDirectory << "\n";

            output.Write(objReport.CStr(), objReport.Size());
            output.Close();
        }

        if (output.Open(FileSystem::PathJoin(GetTempDirectory(), "ObjectFileCycles.txt"), FF_READ | FF_WRITE, FILE_OPEN_ALWAYS))
        {
            SStream ss;
            ss << "Object Cycles within " << objDirectory << "\n";
            
            output.Write(ss.CStr(), ss.Size());
            output.Write(objCycles.CStr(), objCycles.Size());
            output.Close();
        }

        for (auto& row : objRows)
        {
            if (Invalid(row.mFileName.Find("SampleCycle")))
            {
                objectFileCycles += row.mNumCycles;
            }
        }
        numObjects += objRows.size();
    }


    gSysLog.Info(LogMessage("AnalyzeProjectApp processed ") << numHeaders << " headers, " << numSources << " cpp files, " << numObjects << " obj files");
    gSysLog.Info(LogMessage("  Missing Copyright Notices=") << missingCopyrightNotices);
    gSysLog.Info(LogMessage("  Header Cycles=") << headerCycles);
    gSysLog.Info(LogMessage("  Object File Cycles=") << objectFileCycles);
    gSysLog.Info(LogMessage("  Bad Includes=") << badIncludes);

    gSysLog.Info(LogMessage("Copyright Violations"));
    for (auto& file : fixCopyright)
    {
        gSysLog.Info(LogMessage("  ") << file);
    }

    if (!CmdLine::HasArgOption("AnalyzeProject", "nopause"))
    {
        gSysLog.Sync();
        system("pause");
    }

}

} // namespace lf
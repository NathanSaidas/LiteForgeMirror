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

#include "Engine/App/Application.h"
#include "Runtime/Async/PromiseImpl.h"
#include "Core/Memory/MemoryBuffer.h"
#include "Core/Platform/SpinLock.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Utility/CmdLine.h"
#include "Core/Utility/Log.h"

#include <algorithm>

namespace lf {

class DeployBuild;
DECLARE_STRUCT_PTR(DeployCopyOp);
struct DeployCopyOp
{
    void WriteFailure(const LoggerMessage& msg);

    DeployBuild* mSelf;
    String mSource;
    String mDestination;
};

DECLARE_HASHED_CALLBACK(FileCopyPromiseVoid, void);

class DeployBuild : public Application
{
    DECLARE_CLASS(DeployBuild, Application);
private:
    using FileCopyPromise = PromiseImpl<FileCopyPromiseVoid, FileCopyPromiseVoid>;
    void CopyFile(String source, String destination);
    void WaitCopy();

    bool   m_ConfigurationDLL[4];
    String m_ConfigurationNames[4];
    String m_Platforms[1];
    String m_Projects[5];

    String m_RootOutputDirectory;
    String m_CodeOutputDirectory;
    String m_IncludeOutputDirectory;
    String m_LibraryOutputDirectory;

    String m_CodeInputDirectory;

    TVector<PromiseWrapper> m_CopyOperations;
    TVector<LoggerMessage> m_Failures;
    SpinLock m_FailureLock;
public:
    void WriteFailure(const LoggerMessage& message)
    {
        ScopeLock lock(m_FailureLock);
        m_Failures.push_back(message);
    }

    void OnStart() override
    {
        LF_STATIC_ASSERT(LF_ARRAY_SIZE(m_ConfigurationDLL) == LF_ARRAY_SIZE(m_ConfigurationNames));

        // Setup the configuration meta data
        m_ConfigurationNames[0] = "Debug";
        m_ConfigurationNames[1] = "Test";
        m_ConfigurationNames[2] = "Release";
        m_ConfigurationNames[3] = "Final";

        m_ConfigurationDLL[0] = true;
        m_ConfigurationDLL[1] = true;
        m_ConfigurationDLL[2] = false;
        m_ConfigurationDLL[3] = false;

        m_Platforms[0] = "x64";

        m_Projects[0] = "Core";
        m_Projects[1] = "Runtime";
        m_Projects[2] = "AbstractEngine";
        m_Projects[3] = "Service";
        m_Projects[4] = "Engine";

        // Build .dll strings
        TVector<String> dllTargets;
        for (SizeT c = 0; c < LF_ARRAY_SIZE(m_ConfigurationNames); ++c)
        {
            if (!m_ConfigurationDLL[c])
            {
                continue;
            }

            for (SizeT p = 0; p < LF_ARRAY_SIZE(m_Projects); ++p)
            {
                for (SizeT platform = 0; platform < LF_ARRAY_SIZE(m_Platforms); ++platform)
                {
                    dllTargets.push_back(m_Projects[p] + "_" + m_Platforms[platform] + m_ConfigurationNames[c] + ".dll");
                    gTestLog.Debug(LogMessage("Targeting DLL ") << dllTargets.back());
                }
            }
        }

        // OpenSSL DLLs that must be purged.
        dllTargets.push_back("libcrypto-3.dll");
        dllTargets.push_back("libssl-3.dll");

        // [optional] -deploy /Code="..."
        // [optional] -deploy /Lib="..."
        // [optional] -deploy /Include="..."
        // [optional] -deploy /Help or /? 
        // [required] -deploy /Root="..."

        if (CmdLine::HasArgOption("deploy", "help") || CmdLine::HasArgOption("deploy", "?"))
        {
            auto msg = LogMessage("Welcome to DeployBuild help info.");
            msg << "\nDescription:";
            msg << "\n  This is a tool used in conjunction with build scripts to copy the necessary library/dll/header files to a new (or existing) directory which can be used to";
            msg << "\n  write your own application specific code using the LiteForge engine as a library.";
            msg << "\nHere is a list of the following commands.";
            msg << "\n  [required] -deploy /Root=\"...\"      -- A required argument which specifies where the 'deployed' files will built and copied to.";
            msg << "\n  [optional] -deploy /Code=\"...\"      -- Provides an override to the 'Code' directory which is where the .dll files are deployed to. (Default=Code)";
            msg << "\n  [optional] -deploy /Lib=\"...\"       -- Provides an override to the 'Lib' directory which is where the .lib files are deployed to. (Default=Lib)";
            msg << "\n  [optional] -deploy /Include=\"...\"   -- Provides an override to the 'Include' directory which is where the headers files are deployed to. (Default=Include)";
            msg << "\n  [optional] -deploy /Project_Output=\"...\"   -- Provides an override for where all the source .dll/.lib/header files are.";
            msg << "\n  [optional] -deploy /Clean             -- Cleans the Code/Lib/Include directories of the deploy target.";
            msg << "\n  [optional] -deploy /Tool              -- Overrides the deploy target to the tools directory. (Deploys Binary/Executable Only)";
            msg << "\n  [optional] -deploy /Help or /?        -- Shows this help information.";
            msg << "\n\nNote:";
            msg << "\n  The Include and Lib path are assumed to be exclusively used by the Deploy tool.. Any files in that directory will be deleted.";

            gTestLog.Info(msg);
            return;
        }
        if (!CmdLine::GetArgOption("deploy", "project_output", m_CodeInputDirectory)) { m_CodeInputDirectory = ""; }
        m_CodeInputDirectory = FileSystem::PathResolve(FileSystem::PathJoin(FileSystem::GetWorkingPath(), m_CodeInputDirectory));
        gTestLog.Info(LogMessage("  ProjectOutput=") << m_CodeInputDirectory);

        if (CmdLine::HasArgOption("deploy", "tool"))
        {
            String code = m_CodeInputDirectory;
            String toolsDir = FileSystem::PathJoin(FileSystem::PathGetParent(m_CodeInputDirectory), "Tools");

            // Copy .dll
            // Copy .exe
            String source = FileSystem::PathJoin(code, "Game_x64Final.exe");
            String dest = FileSystem::PathJoin(toolsDir, "LiteForgeTool_x64Final.exe");

            CopyFile(source, dest);

            WaitCopy();
            return;
        }

        if (!CmdLine::GetArgOption("deploy", "root", m_RootOutputDirectory))
        {
            gTestLog.Error(LogMessage("DeployBuild requires command-line argument 'root' in order to proceed. Use /? for more information."));
            return;
        }

        if (!CmdLine::GetArgOption("deploy", "code", m_CodeOutputDirectory)) { m_CodeOutputDirectory = "Code"; }
        if (!CmdLine::GetArgOption("deploy", "lib", m_LibraryOutputDirectory)) { m_LibraryOutputDirectory = "Lib"; }
        if (!CmdLine::GetArgOption("deploy", "include", m_IncludeOutputDirectory)) { m_IncludeOutputDirectory = "Include"; }
        const bool doClean = CmdLine::HasArgOption("deploy", "clean");

        m_RootOutputDirectory = FileSystem::PathResolve(m_RootOutputDirectory);
        m_CodeOutputDirectory = FileSystem::PathJoin(m_RootOutputDirectory, m_CodeOutputDirectory);
        m_LibraryOutputDirectory = FileSystem::PathJoin(m_RootOutputDirectory, m_LibraryOutputDirectory);
        m_IncludeOutputDirectory = FileSystem::PathJoin(m_RootOutputDirectory, m_IncludeOutputDirectory);

        

        gTestLog.Info(LogMessage("  Code Directory=") << m_CodeOutputDirectory);
        gTestLog.Info(LogMessage("  Library Directory=") << m_LibraryOutputDirectory);
        gTestLog.Info(LogMessage("  Include Directory=") << m_IncludeOutputDirectory);
        gTestLog.Info(LogMessage("  Creating Directories..."));

        // Remove all the 'content' from the library/include directories. (Clean-Deploy)
        if (!FileSystem::PathDeleteRecursive(m_LibraryOutputDirectory))
        {
            gTestLog.Error(LogMessage("Failed to delete the Library directory, are these files currently opened by another application?"));
            return;
        }

        if (!FileSystem::PathDeleteRecursive(m_IncludeOutputDirectory))
        {
            gTestLog.Error(LogMessage("Failed to delete the Include directory, are these files currently opened by another application?"));
            return;
        }

        if (!FileSystem::PathExists(m_CodeOutputDirectory) && !FileSystem::PathCreate(m_CodeOutputDirectory))
        {
            gTestLog.Error(LogMessage("Failed to create 'Code' directory."));
            return;
        }

        if (!FileSystem::PathExists(m_LibraryOutputDirectory) && !FileSystem::PathCreate(m_LibraryOutputDirectory))
        {
            gTestLog.Error(LogMessage("Failed to create 'Library' directory."));
            return;
        }

        if (!FileSystem::PathExists(m_IncludeOutputDirectory) && !FileSystem::PathCreate(m_IncludeOutputDirectory))
        {
            gTestLog.Error(LogMessage("Failed to create 'Include' directory."));
            return;
        }

        // Clean the directories.
        for (const String& target : dllTargets)
        {
            String fullpath = FileSystem::PathJoin(m_CodeOutputDirectory, target);
            if (FileSystem::FileExists(fullpath) && !FileSystem::FileDelete(fullpath))
            {
                gTestLog.Error(LogMessage("Failed to purge '") << target << "' from the Code directory.");
                return;
            }
        }


        if (!doClean)
        {
            gTestLog.Info(LogMessage("Creating Copy tasks..."));

            // We need to identify all 'include' files in the project
            // We need to identify all 'lib'/'.dll' files to copy over
            TVector<String> codeFiles;
            FileSystem::GetAllFiles(m_CodeInputDirectory, codeFiles);

            String workingPath = m_CodeInputDirectory;
            for (const String& file : codeFiles)
            {
                String extension = FileSystem::PathGetExtension(file);
                if (extension == "h" || extension == "hpp")
                {
                    String source = file;
                    String destination = FileSystem::PathJoin(m_IncludeOutputDirectory, file.SubString(workingPath.Size()));
                    gTestLog.Info(LogMessage("Copy Header: ") << source << " -> " << destination);
                    CopyFile(source, destination);
                }
                else if (extension == "lib")
                {
                    String source = file;
                    String destination = FileSystem::PathJoin(m_LibraryOutputDirectory, file.SubString(workingPath.Size()));
                    gTestLog.Info(LogMessage("Copy Library: ") << source << " -> " << destination);
                    CopyFile(source, destination);
                }
                else if (extension == "dll")
                {
                    String source = file;
                    String destination = FileSystem::PathJoin(m_CodeOutputDirectory, file.SubString(workingPath.Size()));
                    gTestLog.Info(LogMessage("Copy DLL: ") << source << " -> " << destination);
                    CopyFile(source, destination);
                }
            }

            WaitCopy();
        }
        

        if (!m_Failures.empty())
        {
            gTestLog.Error(LogMessage("Deploy Failed!"));
            ScopeLock lock(m_FailureLock);
            for (const auto& msg : m_Failures)
            {
                gTestLog.Error(msg);
            }
        }
        else
        {
            gTestLog.Info(LogMessage("Deploy Complete!"));
        }
        // todo: Right now everything is self contained but we might need third-party libraries...
    }

};
DEFINE_CLASS(DeployBuild) { NO_REFLECTION; }

void DeployCopyOp::WriteFailure(const LoggerMessage& msg) { mSelf->WriteFailure(msg); }

void DeployBuild::CopyFile(String source, String destination)
{
    DeployCopyOpPtr op(LFNew<DeployCopyOp>());
    op->mSelf = this;
    op->mDestination = destination;
    op->mSource = source;

    PromiseWrapper promise = FileCopyPromise([op](Promise* self)
    {
        auto promise = static_cast<FileCopyPromise*>(self);
        // open source
        // open destination
        // write source -> destination
        // close all

        const String& source = op->mSource;
        const String& destination = op->mDestination;

        File sourceFile;
        if (!sourceFile.Open(source, FF_READ | FF_SHARE_READ | FF_SHARE_WRITE, FILE_OPEN_EXISTING))
        {
            op->WriteFailure(LogMessage("Failed to open 'source' file ") << source);
            promise->Reject();
            return;
        }

        String destPath = FileSystem::PathGetParent(destination);
        // Try to create it.. although another thread might create it so check if it exists again after.s
        if (!FileSystem::PathExists(destPath) && !FileSystem::PathCreate(destPath))
        {
            if (!FileSystem::PathExists(destPath))
            {
                op->WriteFailure(LogMessage("Failed to create 'destination' directory ") << destPath);
                promise->Reject();
                return;
            }
        }

        File destFile;
        if (!destFile.Open(destination, FF_WRITE | FF_READ, FILE_OPEN_ALWAYS))
        {
            op->WriteFailure(LogMessage("Failed to open 'destination' file ") << destination);
            promise->Reject();
            return;
        }

        MemoryBuffer buffer;
        buffer.Allocate(static_cast<SizeT>(sourceFile.GetSize()), LF_SIMD_ALIGN);
        
        SizeT bytesRead = sourceFile.Read(buffer.GetData(), buffer.GetSize());
        if (bytesRead != buffer.GetSize())
        {
            op->WriteFailure(LogMessage("Failed to read 'source' file. Bytes Read=") << bytesRead << " but file size is " << buffer.GetSize() << " bytes large");
            promise->Reject();
            return;
        }

        SizeT bytesWritten = destFile.Write(buffer.GetData(), buffer.GetSize());
        if (bytesWritten != buffer.GetSize())
        {
            op->WriteFailure(LogMessage("Failed to write 'destination' file. Bytes Written=") << bytesWritten << " but file size is " << buffer.GetSize() << " bytes large");
            promise->Reject();
            return;
        }


        sourceFile.Close();
        destFile.Close();
        buffer.Free();
        promise->Resolve();
    })
    .Execute();
    m_CopyOperations.push_back(promise);
}

void DeployBuild::WaitCopy()
{
    gTestLog.Info(LogMessage("Waiting for copy to complete..."));
    Async::WaitAll(m_CopyOperations.begin(), m_CopyOperations.end(), [](const PromiseWrapper& p) { return p->IsDone(); });
    for (const auto& op : m_CopyOperations)
    {
        if (!op->IsDone())
        {
            gTestLog.Warning(LogMessage("There are pending copy operations left over. Async::WaitAll failed!"));
            break;
        }
    }
}

} // namespace lf
@ECHO OFF
REM This script builds all projects and configurations.
REM 
REM CommandLine Arguments
REM call BuildAll.bat Rebuild  -- This will 'rebuild' all configurations/projects rather than incremental build.
REM call BuildAll.bat [Help|?] -- This will display help info for this command.
SETLOCAL
REM ==============================================
REM Set our 'working' state
REM ==============================================
SET gWorkingDirectory=%cd%
SET gWorkingDrive=%~d0
SET gVSInstallDrive="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools"
SET gBuildType=%1
SET /A gHelp=0

IF /I "%gBuildType%"=="help" (
   SET /A gHelp=1
)
IF /I "%gBuildType%"=="?" (
   SET /A gHelp=1
)

REM Show help info:
IF /I "%gHelp%" EQU "1" (
   ECHO This script builds all projects and configurations
   ECHO CommandLine Arguments
   ECHO   "call BuildAll.bat Rebuild"  -- This will 'rebuild' all configurations/projects rather than incremental build.
   ECHO   "call BuildAll.bat [Help|?]" -- This will display the help info for this command.
   EXIT /b 0
)

REM Set build type:
IF /I "%gBuildType%"=="Rebuild" (
   SET gBuildType=Rebuild
) ELSE (
   SET gBuildType=Build
)

REM ==============================================
REM Load the visual Studio Developer variables
REM ==============================================
PUSHD %gVSInstallDrive%
call VsDevCmd.bat

REM ==============================================
REM Goto Solution Directory
REM ==============================================
PUSHD %gWorkingDrive%
CD %gWorkingDirectory%
CD ..

REM ==============================================
REM Build Solution
REM ==============================================
ECHO Building x64 Debug
msbuild LiteForge.sln /t:%gBuildType% /p:Configuration=Debug > Temp/BuildAll_x64_Debug.txt
IF %ERRORLEVEL% NEQ 0 (
	ECHO Failed to build x64 Debug see the log in Temp/BuildAll_x64_Debug.txt
)

ECHO Building x64 Test
msbuild LiteForge.sln /t:%gBuildType% /p:Configuration=Test > Temp/BuildAll_x64_Test.txt
IF %ERRORLEVEL% NEQ 0 (
	ECHO Failed to build x64 Test see the log in Temp/BuildAll_x64_Test.txt
)

ECHO Building x64 Release
msbuild LiteForge.sln /t:%gBuildType% /p:Configuration=Release > Temp/BuildAll_x64_Release.txt
IF %ERRORLEVEL% NEQ 0 (
	ECHO Failed to build x64 Release see the log in Temp/BuildAll_x64_Release.txt
)

ECHO Building x64 Final
msbuild LiteForge.sln /t:%gBuildType% /p:Configuration=Final > Temp/BuildAll_x64_Final.txt
IF %ERRORLEVEL% NEQ 0 (
	ECHO Failed to build x64 Final see the log in Temp/BuildAll_x64_Final.txt
)

ECHO Build Complete

CD %gWorkingDirectory%
EXIT /b 0
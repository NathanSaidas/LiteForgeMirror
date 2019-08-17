@ECHO OFF
REM BuildTool
SETLOCAL

IF /I "%1"=="nobuild" (
    ECHO [BuildTool.bat] Skipping build...
) ELSE (
    call BuildAll.bat
    IF %ERRORLEVEL% NEQ 0 (
        EXIT /b 0
    )
)

"../Code/Game_x64Final.exe" -app /type=DeployBuild -deploy /tool /project_output=\"../Code\"
IF %ERRORLEVEL% NEQ 0 (
   ECHO [BuildTool.bat] Failed to deploy tool...
   PAUSE
   EXIT /b 0
)
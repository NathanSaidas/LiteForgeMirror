@ECHO OFF
REM BuildTool...
REM todo: 'target' and 'clean' and 'rebuild' param.
SETLOCAL
SET /A gCleanBuild=0
IF /I "%1"=="clean" (
    SET /A gCleanBuild=1
) ELSE (
    SET /A gCleanBuild=0
)

call BuildAll.bat
IF %ERRORLEVEL% NEQ 0 (
    EXIT /b 0
)
call BuildTool.bat nobuild
IF %ERRORLEVEL% NEQ 0 (
    EXIT /b 0
)

IF /I "%gCleanBuild%" EQU "1" (
"LiteForgeTool_x64Final.exe" -app /type=DeployBuild -deploy /root=\"D:\Game Development\Engine\LiteForgeDevDeployExample\" /project_output=\"../Code\" /clean
) ELSE (
"LiteForgeTool_x64Final.exe" -app /type=DeployBuild -deploy /root=\"D:\Game Development\Engine\LiteForgeDevDeployExample\" /project_output=\"../Code\"
)


IF %ERRORLEVEL% NEQ 0 (
    ECHO [BuildAndDeploy.bat] Failed to deploy...
    PAUSE
    EXIT /b 0
)
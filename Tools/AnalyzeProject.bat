REM Analyze the project files for code cycles

LiteForgeTool_x64Final.exe -app /type=AnalyzeProjectApp -AnalyzeProject /OBJ=\"../Builds/x64Debug\" /SOURCE=\"../Code\"
LiteForgeTool_x64Final.exe -app /type=AnalyzeProjectApp -AnalyzeProject /OBJ=\"../Builds/x64Test\" /SOURCE=\"../Code\"

REM // todo: These fail to analyze the code.
REM LiteForgeTool_x64Final.exe -app /type=AnalyzeProjectApp -AnalyzeProject /OBJ=\"../Builds/x64Release\" /SOURCE=\"../Code\"
REM LiteForgeTool_x64Final.exe -app /type=AnalyzeProjectApp -AnalyzeProject /OBJ=\"../Builds/x64Final\" /SOURCE=\"../Code\"
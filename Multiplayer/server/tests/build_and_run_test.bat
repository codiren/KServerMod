@echo off
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
call "%VCVARS%"

echo Compiling Serialization Test...
cl.exe /nologo /EHsc /O2 serialization_test.cpp /Fe:serialization_test.exe

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation failed!
    exit /b 1
)

echo Running Test...
serialization_test.exe

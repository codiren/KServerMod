@echo off
set "ROOT=%~dp0"

:: -----------------------------------------------------------------------------
:: SERVER BUILD
:: -----------------------------------------------------------------------------
echo ========================================
echo BUILDING SERVER
echo ========================================
set "PROJ_DIR=%ROOT%KenshiLib_Examples\Multiplayer\server"
set "IMGUI_DIR=%ROOT%KenshiLib_Examples_deps\imgui"
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

if "%VSCMD_ARG_TGT_ARCH%" == "x64" goto skip_vcvars
if not exist "%VCVARS%" (
    echo [ERROR] VS2022 vcvars64.bat not found at %VCVARS%
    exit /b 1
)
call "%VCVARS%"
:skip_vcvars

echo [SERVER] Cleaning old build files...
if exist "%PROJ_DIR%\server.obj" del /q "%PROJ_DIR%\server.obj"
if exist "%PROJ_DIR%\server.exe" del /q "%PROJ_DIR%\server.exe"

echo [SERVER] Compiling...
pushd "%PROJ_DIR%"

if not exist imgui.obj (
    echo [SERVER] First-time setup: Compiling ImGui cores...
    cl.exe /nologo /c /EHsc /O2 /MP /I"%IMGUI_DIR%" /I"%IMGUI_DIR%\backends" ^
        "%IMGUI_DIR%\imgui.cpp" ^
        "%IMGUI_DIR%\imgui_draw.cpp" ^
        "%IMGUI_DIR%\imgui_widgets.cpp" ^
        "%IMGUI_DIR%\imgui_tables.cpp" ^
        "%IMGUI_DIR%\backends\imgui_impl_win32.cpp" ^
        "%IMGUI_DIR%\backends\imgui_impl_dx11.cpp"
)

cl.exe /nologo /c /EHsc /O2 /I"%IMGUI_DIR%" /I"%IMGUI_DIR%\backends" server.cpp
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] SERVER Compilation FAILED!
    popd
    exit /b 1
)

link.exe /nologo /SUBSYSTEM:WINDOWS /OUT:server.exe ^
    server.obj imgui.obj imgui_draw.obj imgui_widgets.obj imgui_tables.obj ^
    imgui_impl_win32.obj imgui_impl_dx11.obj ^
    ws2_32.lib user32.lib gdi32.lib comctl32.lib d3d11.lib d3dcompiler.lib

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] SERVER Link FAILED!
    popd
    exit /b 1
)
echo [SERVER] Build successful!
popd

:: -----------------------------------------------------------------------------
:: CLIENT BUILD (Multiplayer.dll)
:: -----------------------------------------------------------------------------
echo ========================================
echo BUILDING CLIENT (Multiplayer.dll)
echo ========================================

set "PROJ_DIR=%ROOT%KenshiLib_Examples\Multiplayer"
set "DEPS=%ROOT%KenshiLib_Examples_deps"
set "LIB=%DEPS%\KenshiLib"
set "BOOST=%DEPS%\boost_1_60_0"
set "WIN7INC=C:\Program Files\Microsoft SDKs\Windows\v7.1\Include"
set "VS10INC=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\include"
set "INC=%LIB%\Include;%LIB%\Include\ogre;%DEPS%;%BOOST%;%WIN7INC%;%VS10INC%"
set "WIN7LIB=C:\Program Files\Microsoft SDKs\Windows\v7.1\Lib\x64"
set "VS10LIB=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib\amd64"
set "KLIB=%LIB%\Libraries"
set "LIBS=%KLIB%;%WIN7LIB%;%VS10LIB%;%BOOST%\stage\lib"
set "MSB=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

pushd "%PROJ_DIR%"
"%MSB%" Multiplayer.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:IncludePath="%INC%" /p:LibraryPath="%LIBS%" /p:TrackFileAccess=false
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CLIENT Build failed!
    popd
    exit /b %ERRORLEVEL%
)
echo [CLIENT] Build successful!
popd

echo.
echo ========================================
echo ALL BUILDS SUCCESSFUL. NO APPS LAUNCHED.
echo ========================================

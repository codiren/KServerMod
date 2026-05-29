@echo off
set "ROOT=%~dp0"
set "PROJ_DIR=%ROOT%KenshiLib_Examples\Multiplayer"
set "DEPS=%ROOT%KenshiLib_Examples_deps"
set "LIB=%DEPS%\KenshiLib"
set "BOOST=%DEPS%\boost_1_60_0"
set "WIN7INC=C:\Program Files\Microsoft SDKs\Windows\v7.1\Include"
set "VS10INC=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\include"
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
set "INC=%LIB%\Include;%LIB%\Include\ogre;%DEPS%;%BOOST%;%WIN7INC%;%VS10INC%"

set "WIN7LIB=C:\Program Files\Microsoft SDKs\Windows\v7.1\Lib\x64"
set "VS10LIB=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib\amd64"
set "KLIB=%LIB%\Libraries"
set "LIBS=%KLIB%;%WIN7LIB%;%VS10LIB%;%BOOST%\stage\lib"

set "PATH=C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\x64;C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\x86_amd64;%PATH%"

set "MSB=C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"

echo ========================================
echo BUILDING SERVER
echo ========================================
set "SERVER_DIR=%PROJ_DIR%\server"
set "IMGUI_DIR=%DEPS%\imgui"

call "%VCVARS%"

pushd "%SERVER_DIR%"
echo [SERVER] Compiling...
if not exist imgui.obj (
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
popd

echo ========================================
echo BUILDING CLIENT (Multiplayer.dll)
echo ========================================
pushd "%PROJ_DIR%"
"%MSB%" Multiplayer.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:IncludePath="%INC%" /p:LibraryPath="%LIBS%" /p:TrackFileAccess=false
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    popd
    exit /b %ERRORLEVEL%
)

echo.
echo Copying Build results...
copy /y "x64\Release\Multiplayer.dll" "Multiplayer\Multiplayer.dll"
copy /y "server\server.exe" "Multiplayer\server.exe"

echo Deploying to Kenshi mods...
set "TARGET_MOD_DIR=C:\games\Kenshi_multiplayer\mods\Multiplayer"
if exist "%TARGET_MOD_DIR%" rd /s /q "%TARGET_MOD_DIR%"
mkdir "%TARGET_MOD_DIR%"
xcopy /e /y "Multiplayer\*" "%TARGET_MOD_DIR%\"
popd

echo Starting Server...
start "" "%TARGET_MOD_DIR%\server.exe"

echo Running Kenshi...
cd /d "C:\games\Kenshi_multiplayer"
start "" "kenshi_GOG_1.0.65_x64.exe"

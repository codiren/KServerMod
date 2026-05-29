@echo off
set "ROOT=%~dp0"
set "PROJ_DIR=%ROOT%KenshiLib_Examples\Multiplayer\server"
set "IMGUI_DIR=%ROOT%KenshiLib_Examples_deps\imgui"

:: VS2022 Environment
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

:: Skip vcvars if already in environment to save time
if "%VSCMD_ARG_TGT_ARCH%" == "x64" goto skip_vcvars
if not exist "%VCVARS%" (
    echo [ERROR] VS2022 vcvars64.bat not found at %VCVARS%
    pause
    exit /b 1
)
call "%VCVARS%"
:skip_vcvars

set "PROJ_DIR=%ROOT%KenshiLib_Examples\Multiplayer\server"

echo ========================================
echo DEPLOYING AND LAUNCHING SERVER
echo ========================================
taskkill /F /IM server.exe /T > nul 2>&1

echo [SERVER] Cleaning old build files...
if exist "%PROJ_DIR%\server.obj" del /q "%PROJ_DIR%\server.obj"
if exist "%PROJ_DIR%\server.exe" del /q "%PROJ_DIR%\server.exe"

echo [SERVER] Compiling...
pushd "%PROJ_DIR%"

:: 1. Pre-compile ImGui if missing
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

:: 2. Compile server.cpp (fast)
cl.exe /nologo /c /EHsc /O2 /I"%IMGUI_DIR%" /I"%IMGUI_DIR%\backends" server.cpp
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Compilation FAILED!
    echo.
    pause
    popd
    exit /b 1
)

:: 3. Link everything (fast)
link.exe /nologo /SUBSYSTEM:WINDOWS /OUT:server.exe ^
    server.obj imgui.obj imgui_draw.obj imgui_widgets.obj imgui_tables.obj ^
    imgui_impl_win32.obj imgui_impl_dx11.obj ^
    ws2_32.lib user32.lib gdi32.lib comctl32.lib d3d11.lib d3dcompiler.lib

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Link FAILED!
    echo.
    pause
    popd
    exit /b 1
)

echo [SERVER] Build successful!

:: Deploying
set "TARGET_1=C:\games\Kenshi_multiplayer\mods\Multiplayer"
set "TARGET_2=C:\games\Kenshi_multiplayer_player_2\mods\Multiplayer"
set "SOURCE_MOD_DIR=%ROOT%KenshiLib_Examples\Multiplayer\Multiplayer"
set "CLIENT_DLL=%ROOT%KenshiLib_Examples\Multiplayer\x64\Release\Multiplayer.dll"

echo [DEPLOY] Copying client DLL...
if exist "%CLIENT_DLL%" (
    if exist "%SOURCE_MOD_DIR%" copy /y "%CLIENT_DLL%" "%SOURCE_MOD_DIR%\Multiplayer.dll" > nul
    if exist "%TARGET_1%" copy /y "%CLIENT_DLL%" "%TARGET_1%\Multiplayer.dll" > nul
    if exist "%TARGET_2%" copy /y "%CLIENT_DLL%" "%TARGET_2%\Multiplayer.dll" > nul
) else (
    echo [WARNING] Client DLL not found at %CLIENT_DLL%
    echo [WARNING] Run build.bat first to build the client!
)

echo [DEPLOY] Copying server.exe...
if exist "%TARGET_1%" copy /y "server.exe" "%TARGET_1%\server.exe" > nul
if exist "%TARGET_2%" copy /y "server.exe" "%TARGET_2%\server.exe" > nul
if exist "%SOURCE_MOD_DIR%" copy /y "server.exe" "%SOURCE_MOD_DIR%\server.exe" > nul

echo [SERVER] Starting Kenshi Multiplayer Server...
cd /d "C:\games\Kenshi_multiplayer\mods\Multiplayer"
start "" server.exe

popd

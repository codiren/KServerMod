@echo off
set "ROOT=c:\projects\kenshi_multiplayer\v0.4\"
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

cd /d "%PROJ_DIR%"
"%MSB%" Multiplayer.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:IncludePath="%INC%" /p:LibraryPath="%LIBS%" /p:TrackFileAccess=false > client_build_error.log
type client_build_error.log

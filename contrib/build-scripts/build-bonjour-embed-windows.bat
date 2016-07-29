@ECHO off

set ME=%0
set DIR=%~dp0
set MSVC_VER=""

if "%VSINSTALLDIR%" == "" (
	echo.
	echo %VSINSTALLDIR is not defined, run from within Visual Studio Command Prompt.
	echo.
	goto :DONE
)

cl.exe 2>&1 |findstr "Version\ 18.00" > nul
if %errorlevel% == 0 (
	set MSVC_VER="-1800"
)

cl.exe 2>&1 |findstr "Version\ 16.00" > nul
if %errorlevel% == 0 (
	set MSVC_VER="-1600"
)

if "%MSVC_VER%" == "" (
	echo.
	echo Unknown MSVC_VER %MSVC_VER%.
	echo.
	goto :DONE
)

echo %LIB% |find "LIB\amd64;" > nul
if %errorlevel% == 0 (
	set DEST_DIR="%DIR%..\prebuilt\windows-x86_64\msvc%MSVC_VER%"
	set CPU_ARCH=x86_64
	goto :ARCH_FOUND
)

echo %LIB% |find "LIB;" > nul
if %errorlevel% == 0 (
	set DEST_DIR="%DIR%..\prebuilt\windows-x86\msvc%MSVC_VER%"
	set CPU_ARCH=x86
	goto :ARCH_FOUND
)

:ARCH_FOUND

if "%DEST_DIR%" == "" (
	echo.
	echo Unknown Platform %Platform%.
	echo.
	goto :DONE
)

IF NOT EXIST "mDNSResponder-333.10\mDNSWindows\mDNSWin32.c" (
	echo.
	echo Cannot find mDNSResponder-333.10\mDNSWindows\mDNSWin32.c
	echo Run script from within archives directory:
	echo archives $ ..\%ME%
	echo.
	goto :DONE
)

devenv /upgrade mDNSEmbedded.vs10.330.10/mDNSEmbedded.sln

mkdir %DEST_DIR%\include
mkdir %DEST_DIR%\lib

if "%CPU_ARCH%" == "x86_64" (
  devenv /build "release|x64" mDNSEmbedded.vs10.330.10\mDNSEmbedded.sln /project mDNSEmbedded
	devenv /build "debug|x64" mDNSEmbedded.vs10.330.10\mDNSEmbedded.sln /project mDNSEmbedded

  copy mDNSEmbedded.vs10.330.10\x64\Debug\mDNSEmbedded.lib %DEST_DIR%\lib\mDNSEmbedded_d.lib
  copy mDNSEmbedded.vs10.330.10\x64\Debug\mdnsresponder.vc120.pdb %DEST_DIR%\lib\mdnsresponder.vc120.pdb
  copy mDNSEmbedded.vs10.330.10\x64\Release\mDNSEmbedded.lib %DEST_DIR%\lib\mDNSEmbedded.lib
)

if "%CPU_ARCH%" == "x86" (
	devenv /build "debug|Win32" mDNSEmbedded.vs10.330.10\mDNSEmbedded.sln /project mDNSEmbedded	
  devenv /build "release|Win32" mDNSEmbedded.vs10.330.10\mDNSEmbedded.sln /project mDNSEmbedded

	copy mDNSEmbedded.vs10.330.10\Debug\mDNSEmbedded.lib %DEST_DIR%\lib\mDNSEmbedded_d.lib
  copy mDNSEmbedded.vs10.330.10\Debug\mdnsresponder.vc120.pdb %DEST_DIR%\lib\mdnsresponder.vc120.pdb  
  copy mDNSEmbedded.vs10.330.10\Release\mDNSEmbedded.lib %DEST_DIR%\lib\mDNSEmbedded.lib
)

mkdir %DEST_DIR%\include\bonjour
copy mDNSResponder-333.10\mDNSShared\CommonServices.h %DEST_DIR%\include\bonjour
copy mDNSEmbedded-330.10\mDNSShared\dns_sd.h %DEST_DIR%\include\bonjour
copy mDNSResponder-333.10\mDNSCore\DNSCommon.h %DEST_DIR%\include\bonjour
copy mDNSResponder-333.10\mDNSCore\mDNSDebug.h %DEST_DIR%\include\bonjour
copy mDNSResponder-333.10\mDNSCore\mDNSEmbeddedAPI.h %DEST_DIR%\include\bonjour
copy mDNSResponder-333.10\mDNSWindows\mDNSWin32.h %DEST_DIR%\include\bonjour
copy mDNSResponder-333.10\mDNSPosix\mDNSPosix.h %DEST_DIR%\include\bonjour
copy mDNSResponder-333.10\mDNSCore\uDNS.h %DEST_DIR%\include\bonjour

:DONE
pause
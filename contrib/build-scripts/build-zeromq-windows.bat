@ECHO off

set ME=%0
set DIR=%~dp0

if "%VSINSTALLDIR%" == "" (
	echo.
	echo %VSINSTALLDIR is not defined, run from within Visual Studio Command Prompt.
	echo.
	goto :DONE
)

echo %LIB% |find "LIB\amd64;" > nul
if %errorlevel% == 0 (
	set DEST_DIR="%DIR%..\prebuilt\windows-x86_64\msvc"
	set CPU_ARCH=x86_64
	goto :ARCH_FOUND
)

echo %LIB% |find "LIB;" > nul
if %errorlevel% == 0 (
	set DEST_DIR="%DIR%..\prebuilt\windows-x86\msvc"
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

IF NOT EXIST "src\zmq.cpp" (
	echo.
	echo Cannot find src\arabica.cpp
	echo Run script from within arabica directory:
	echo arabica $ ..\%ME%
	echo.
	goto :DONE
)

devenv /upgrade builds/msvc/msvc10.sln

mkdir %DEST_DIR%\include
mkdir %DEST_DIR%\lib

if "%CPU_ARCH%" == "x86_64" (
  devenv /build "release|x64" builds\msvc\msvc10.sln /project libzmq
	devenv /build "debug|x64" builds\msvc\msvc10.sln /project libzmq

  copy bin\x64\libzmq_d.lib %DEST_DIR%\lib\libzmq_d.lib
  copy bin\x64\libzmq_d.pdb %DEST_DIR%\lib\libzmq_d.pdb
  copy bin\x64\libzmq.lib %DEST_DIR%\lib\libzmq.lib
  xcopy include\*.h %DEST_DIR%\include /s /e
)

if "%CPU_ARCH%" == "x86" (
	devenv /build "debug|Win32" builds\msvc\msvc10.sln /project libzmq	
	copy bin\Win32\libzmq_d.lib %DEST_DIR%\lib\libzmq_d.lib
  copy bin\Win32\libzmq.pdb %DEST_DIR%\lib\libzmq_d.pdb
  
  devenv /build "release|Win32" builds\msvc\msvc10.sln /project libzmq
  copy bin\Win32\libzmq.lib %DEST_DIR%\lib\libzmq.lib
  xcopy include\*.h %DEST_DIR%\include /s /e
  
)

:DONE
pause
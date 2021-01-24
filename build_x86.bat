@echo off

pushd
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat" x86
popd

pushd
call build_no.bat
rem type .\Sources\build_no.h
rem pause

call msbuild.exe .\Trial\pcb_test.vcxproj /t:Clean;Build /p:"Configuration=Debug";"Platform=Win32"
if errorlevel 1 goto end

call msbuild.exe .\Libraries\CANAPI\uvcanpcb.vcxproj /t:Clean;Build /p:"Configuration=Release_dll";"Platform=Win32"
if errorlevel 1 goto end

call msbuild.exe .\Libraries\CANAPI\uvcanpcb.vcxproj /t:Clean;Build /p:"Configuration=Release_lib";"Platform=Win32"
if errorlevel 1 goto end

call msbuild.exe .\Libraries\UVPCAN\uvpcan.vcxproj /t:Clean;Build /p:"Configuration=Release_dll";"Platform=Win32"
if errorlevel 1 goto end

call msbuild.exe .\Libraries\UVPCAN\uvpcan.vcxproj /t:Clean;Build /p:"Configuration=Release_lib";"Platform=Win32"
if errorlevel 1 goto end

echo Copying artifacts...
set BIN=".\Binaries"
if not exist %BIN% mkdir %BIN%
set BIN="%BIN%\x86"
if not exist %BIN% mkdir %BIN%
copy /Y .\Libraries\CANAPI\Release_dll\u3canpcb.dll %BIN%
copy /Y .\Libraries\CANAPI\Release_dll\u3canpcb.lib %BIN%
copy /Y .\Libraries\UVPCAN\Release_dll\uvpcan.dll %BIN%
copy /Y .\Libraries\UVPCAN\Release_dll\uvpcan.lib %BIN%
set BIN="%BIN%\lib"
if not exist %BIN% mkdir %BIN%
copy /Y .\Libraries\CANAPI\Release_lib\u3canpcb.lib %BIN%
copy /Y .\Libraries\UVPCAN\Release_lib\uvpcan.lib %BIN%
copy /Y .\Sources\PCAN_Basic\x86\PCANBasic.lib %BIN%
echo Static libraries (x86) > %BIN%\readme.txt

set INC=".\Includes"
if not exist %INC% mkdir %INC%
copy /Y .\Sources\PCAN*.h %INC%
copy /Y .\Sources\CANAPI\CANAPI*.h %INC%
copy /Y .\Sources\CANAPI\can_api.h %INC%

:end
popd
pause

@echo off

pushd
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat" x64
popd

pushd
call build_no.bat
rem type .\Sources\build_no.h
rem pause

call msbuild.exe .\Trial\pcb_test.vcxproj /t:Clean;Build /p:"Configuration=Debug";"Platform=x64"
if errorlevel 1 goto end

call msbuild.exe .\Libraries\CANAPI\uvcanpcb.vcxproj /t:Clean;Build /p:"Configuration=Release_dll";"Platform=x64"
if errorlevel 1 goto end

call msbuild.exe .\Libraries\CANAPI\uvcanpcb.vcxproj /t:Clean;Build /p:"Configuration=Release_lib";"Platform=x64"
if errorlevel 1 goto end

call msbuild.exe .\Libraries\UVPCAN\uvpcan.vcxproj /t:Clean;Build /p:"Configuration=Release_dll";"Platform=x64"
if errorlevel 1 goto end

call msbuild.exe .\Libraries\UVPCAN\uvpcan.vcxproj /t:Clean;Build /p:"Configuration=Release_lib";"Platform=x64"
if errorlevel 1 goto end

echo Copying artifacts...
set BIN=".\Binaries"
if not exist %BIN% mkdir %BIN%
set BIN="%BIN%\x64"
if not exist %BIN% mkdir %BIN%
copy .\Libraries\CANAPI\x64\Release_dll\u3canpcb.dll %BIN%
copy .\Libraries\CANAPI\x64\Release_dll\u3canpcb.lib %BIN%
copy .\Libraries\UVPCAN\x64\Release_dll\uvpcan.dll %BIN%
copy .\Libraries\UVPCAN\x64\Release_dll\uvpcan.lib %BIN%
set BIN="%BIN%\lib"
if not exist %BIN% mkdir %BIN%
copy .\Libraries\CANAPI\x64\Release_lib\u3canpcb.lib %BIN%
copy .\Libraries\UVPCAN\x64\Release_lib\uvpcan.lib %BIN%
echo Static library (x64) > %BIN%\readme.txt

set INC=".\Includes"
if not exist %INC% mkdir %INC%
copy .\Sources\PCAN*.h %INC%
copy .\Sources\CANAPI\CANAPI*.h %INC%
copy .\Sources\CANAPI\can_api.h %INC%

:end
popd
pause

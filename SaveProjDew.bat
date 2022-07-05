@echo off
rem 
rem -- Copies latest version of %Proj% to
rem -- a directory named %Proj%YYMMDD
rem
rem    Copies to:
rem       Saved\
rem       G:\Backups\%Proj%\
rem       \\Red\redd\Backups\%Proj%
rem       Backups\
rem
rem    09/14/10 DTE Created
rem
set Proj=dew
set ProjHome=C:\D\Projects\%Proj%
set Dest1=%USERPROFILE%\Box Sync\Devel\Projects\Backups\%Proj%
set Dest2=%ProjHome%\Backups
set TmpDir="%ProjHome%\tmp"
set dir=%Proj%%COMPUTERNAME%%date:~12,2%%date:~4,2%%date:~7,2%

if not exist "%TmpDir%" mkdir "%TmpDir%"
pushd "%TmpDir%"

if not exist %dir%.zip goto checkdeldir
echo.
set/p ans="Do you wish to delete file %dir%.zip?"

if "%ans%" == "Y" goto dodel
if "%ans%" == "y" goto dodel
goto no

:dodel
del %dir%.zip

:checkdeldir
if not exist %dir% goto builddir
rmdir/s/q %dir%

:builddir
mkdir %dir%

rem -- Copy %Proj% files
xcopy/f/q %ProjHome%\%Proj%.sln %dir%\
xcopy/f/q %ProjHome%\%Proj%.vcxproj %dir%\
xcopy/f/q %ProjHome%\%Proj%.vcxproj.filters %dir%\
xcopy/f/q %ProjHome%\RenameDirsDew.bat %dir%\
xcopy/f/q %ProjHome%\SaveProjDew.bat %dir%\
rem xcopy/f/q %ProjHome%\%Proj%.vcxproj.user %dir%\

echo Copying src...
xcopy/e/q %ProjHome%\Src %dir%\Src\
xcopy/e/q %ProjHome%\Run %dir%\Run\
xcopy/e/q %ProjHome%\Etc %dir%\Etc\
xcopy/e/q %ProjHome%\.git %dir%\.git\
rem xcopy/e/q %ProjHome%\js %dir%\js\
xcopy/e/q %ProjHome%\vsc %dir%\vsc\
rem xcopy/e/q %ProjHome%\Headers %dir%\Headers\

echo Copied %Proj% files from %COMPUTERNAME% %date% %time:~0,8% >%dir%\PackInfo.txt
type %dir%\PackInfo.txt

echo Zipping...
pushd %dir%
C:\D\bin\zip -q -r -S ..\%dir%.zip *
popd

:CopyDest1
if "%Dest1%" == "" goto CopyDest2

if not exist "%Dest1%" mkdir "%Dest1%"

echo Copying to %Dest1%...
Copy/y %dir%.zip "%Dest1%"

:CopyDest2
if "%Dest2%" == "" goto CopyDone

if not exist "%Dest2%" mkdir "%Dest2%"

echo Copying to %Dest2%...
Copy/y %dir%.zip "%Dest2%"

:CopyDone
echo --- rmdir/q/s %dir%
rmdir/q/s %dir%
del %dir%.zip
popd

echo.
echo Backup of %Proj% copied to:
:msgDest1
if "%Dest1%" == "" goto msgDest2
echo   %Dest1%\%dir%.zip
:msgDest2
if "%Dest2%" == "" goto msgDone
echo   %Dest2%\%dir%.zip
:msgDone
echo.

goto done

:no
echo No Copy done.

:done
echo.
CHOICE /T 4 /C yn /D n /M "Continue "
rem pause




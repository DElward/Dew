@echo off
rem 
rem -- Renames latest version of %Proj%
rem
rem    08/08/20 DTE Created
rem
set Proj=Dew
set OldDir=OldDirs
set ProjHome=C:\D\Projects\%Proj%
set RunHome=%ProjHome%\Run
set SrcHome=%ProjHome%\Src
set EtcHome=%ProjHome%\Etc
set dat=%date:~12,2%%date:~4,2%%date:~7,2%
set TgtHome=%OldDir%\%Proj%%dat%

pushd %ProjHome%

if not exist %OldDir% mkdir %OldDir%

rem if exist %TgtHome% rmdir/s/q %TgtHome%

if not exist %TgtHome% mkdir %TgtHome%

move %ProjHome%\vsc %TgtHome%
rem if errorlevel 1 goto fail

move %SrcHome% %TgtHome%
rem if errorlevel 1 goto fail

move %RunHome% %TgtHome%
rem if errorlevel 1 goto fail

move %EtcHome% %TgtHome%
rem if errorlevel 1 goto fail

goto done

:fail
echo ################################
echo #### FAIL: %0
echo ################################
pause
exit/b 1

:done
echo.
CHOICE /T 4 /C yn /D n /M "Continue "




@REM
@REM Copyright (c)Samsung Elec. co, LTD.  All rights reserved.
@REM
@echo off

echo ----------------------------------------------
echo %_TARGETPLATROOT%\target\%_TGTCPU%\%WINCEDEBUG%
echo ----------------------------------------------

copy /Y %_TARGETPLATROOT%\target\%_TGTCPU%\%WINCEDEBUG%\*.* %_FLATRELEASEDIR%

copy /Y %_TARGETPLATROOT%\FILES\*.* %_FLATRELEASEDIR%


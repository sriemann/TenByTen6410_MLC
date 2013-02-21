@echo off
rem
rem	Copyright (c) Marvell Semiconductor, Inc.  All rights reserved. 
rem
rem	filename: buildprj.bat
rem	Description: Build the WinCE project
rem
rem =================================================================

IF "%1"=="clean" (
	goto :CLEAN
)

IF "%1"=="" (
	goto :HELP
)

IF "%1"=="help" (
	goto :HELP
)

IF "%1"=="cfg" (
	goto CFG
)

IF "%1"=="cab" (
	goto CAB
)

IF "%1"=="all" (
	goto :ALL
)

REM
REM	Don't know what to do. Display the help image
REM

cls
echo ***** Error *****
echo Invalid CMD: "%0 [%1]"
echo *****************
goto :HELP

REM ============================================================================
:CFG

	call .\SYS\buildcfg.bat

goto :END

REM ============================================================================
:CAB
	IF NOT exist .config (
		echo **** Error ****
		echo 	.config is missing. Run buildcfg.bat first
		goto :END
	)

	call .\sys\mysetenv.bat

	echo I/O Interface: %DEVIF%
	echo ChipId: %CHIPID%

	Echo Making the Directory
	IF NOT EXIST .\BIN\%DEVIF%%CHIPID% mkdir .\BIN\%DEVIF%%CHIPID%

	ECHO Copy INF file...
	copy /y .\SYS\INF\%DEVIF%%CHIPID%.inf .\BIN\%DEVIF%%CHIPID%

	ECHO Copy DLL file...
	copy /y .\WLAN\obj\ARMV4I\%WINCEDEBUG%\%DEVIF%%CHIPID%.dll .\BIN\%DEVIF%%CHIPID%

	pushd .
	rem -------------------------
	cd /d .\BIN\%DEVIF%%CHIPID%

	ECHO Running CABWiz...

	IF %_WINCEOSVER%==501 (
		"d:\Program Files\Platform Builder for Windows Mobile\5.00\CEPB\BIN\cabwiz" %DEVIF%%CHIPID%.inf
	) ELSE (
		cabwiz %DEVIF%%CHIPID%.inf
	)

	dir *.cab
	rem -------------------------
	popd

	ECHO End Of Building .cab file ...

goto :END

REM ============================================================================
:ALL
	IF NOT exist .config (
		echo **** Error ****
		echo 	.config is missing. Run buildcfg.bat first
		goto :END
	)

	call .\sys\mysetenv.bat

	echo I/O Interface: %DEVIF%
	echo ChipId: %CHIPID%
	echo RF: %RF%
rem	echo HostType: %HOSTTYPE%
	echo Mode: %80211MODE%

	build

	IF %_WINCEOSVER%==501 (
		echo signing the driver ...
		sign WLAN\obj\ARMV4I\retail\%DEVIF%%CHIPID%.dll
	)

	IF %DEVIF%==GSPI (
		echo copying files to %_FLATRELEASEDIR%...

		del /f/q %_FLATRELEASEDIR%\gspi*.*
		copy /y .\WLAN\obj\ARMV4I\retail\gspi%CHIPID%.dll %_FLATRELEASEDIR%\.
	)

goto :END

REM ============================================================================
:CLEAN
	echo Cleaning ...
	del /q Build.*

	IF EXIST .\sign.log			del /q sign.log

	IF EXIST .\WLAN\obj			rd /q/s .\WLAN\obj

	IF EXIST .\IO\GSPI\PXA270\obj	rd /q/s .\IO\GSPI\PXA270\obj
	IF EXIST .\IO\SDIO\PXA270\obj	rd /q/s .\IO\SDIO\PXA270\obj
	IF EXIST .\IO\CF\X86\obj		rd /q/s .\IO\CF\X86\obj
	IF EXIST .\IO\USB\X86\obj		rd /q/s .\IO\USB\X86\obj

	IF EXIST .\IF\IF_GSPI\obj		rd /q/s .\IF\IF_GSPI\obj
	IF EXIST .\IF\IF_SDIO\obj		rd /q/s .\IF\IF_SDIO\obj
	IF EXIST .\IF\IF_CF\obj			rd /q/s .\IF\IF_CF\obj
	IF EXIST .\IF\IF_USB\obj		rd /q/s .\IF\IF_USB\obj

goto :End

REM ============================================================================
:HELP
	echo Usage: (case sensibility)
	echo "buildprj [CMD] "
	echo "	[CMD] = [all | clean | cfg | cab | help]"
	echo "	all: the project will be build without cleaning the intermedia files"
	echo "	clean: Clean up the intermedia files"
	echo "	cfg: Create the .config file"
	echo "	cab: Build the .cab file"
	echo "	help: Display this message"

goto :End

REM ============================================================================
:END

    echo End_Of_Building....

:EXIT


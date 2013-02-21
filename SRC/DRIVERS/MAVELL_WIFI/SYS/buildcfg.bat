@echo off
rem	Copyright (c) Marvell Semiconductor, Inc.  All rights reserved. 
rem
rem	filename: buildcfg.bat
rem	Description: To create the .config to set the interface parameters
rem 
rem	================================================================= 

setlocal
echo ======================================
echo Setup the building parameter

IF EXIST .\.config (
	del /q	.config
)

set INVALID_DEVIF=INVALID
SET INVALID_CHIPID=INVALID
SET INVALID_RF=INVALID
SET INVALID_HOSTTYPE=INVALID
SET INVALID_MODE=INVALID

set DEVIF=""
set CHIPID=""
set RF=""
set HOSTTYPE=""
set MODE=""

rem	================================================================= 
rem :LABEL_DEVIF
set /p DEVIF=[Please input the device interface (SDIO / GSPI):] 

FOR %%i in (SDIO, sdio) do (
	IF %DEVIF% == %%i (
		set DEVIF=SDIO
		SET INVALID_DEVIF=NO
		goto LABEL_CHIPID
	)
)

FOR %%i in (GSPI, gspi) do (
	IF %DEVIF% == %%i (
		SET DEVIF=GSPI
		SET INVALID_DEVIF=NO
		goto LABEL_CHIPID
	)
)

IF %INVALID_DEVIF% == INVALID (
	SET INVALID_DEVIF=YES
	goto Error
)

rem	================================================================= 
:LABEL_CHIPID
set /p CHIPID=[Please input the chip_id (8305 / 8381 / 8385 / 8686 / 8388):] 

FOR %%i in (8305, 8381, 8385, 8686, 8388) do (
	IF %CHIPID% == %%i (
		SET INVALID_CHIPID = NO
		goto LABEL_RF
	)
)

IF %INVALID_CHIPID% == INVALID (
	set INVALID_CHIPID=YES
	goto Error
)

rem	================================================================= 
:LABEL_RF
set /p RF=[Please input the rf_id (8010 / 8015 / 8030 / 8031):] 

FOR %%i in (8010, 8015, 8030, 8031) do (
	IF %RF% == %%i (
		SET INVALID_RF = NO
		goto LABEL_HOST
	)
)

IF %INVALID_RF% == INVALID (
	set INVALID_RF=YES
	goto Error
)

rem	================================================================= 
:LABEL_HOST
IF %DEVIF% == SDIO (
	SET HOSTTYPE=S3C2443
	SET INVALID_HOST = NO
	goto LABEL_MODE
)

set /p HOSTTYPE=[Please input the host-type (PXA270 / S3C2443):] 

FOR %%i in (PXA270, S3C2443, S3C2443) do (
	IF %HOSTTYPE% == %%i (
		SET HOSTTYPE=S3C2443
		SET INVALID_HOST = NO
		goto LABEL_MODE
	)
)

IF %INVALID_HOST% == INVALID (
	set INVALID_HOST=YES
	goto Error
)

rem	================================================================= 
:LABEL_MODE
set /p MODE=[Please input the mode (B / BG / A / ABG):] 

FOR %%i in (B, BG, A, ABG) do (
	IF %MODE% == %%i (
		SET INVALID_MODE = NO
		goto LABEL_BUILDCFG
	)
)

IF %INVALID_MODE% == INVALID (
	set INVALID_MODE=YES
	goto Error
)

rem	================================================================= 
:LABEL_BUILDCFG
	echo DEVINTRF: %DEVIF% > .config
	echo CHIPSID: %CHIPID% >> .config
	echo RF: %RF% >> .config
	echo HOST: %HOSTTYPE% >> .config
	echo MODE: %MODE% >> .config

GOTO End

rem	================================================================= 
:Error
echo 	***** Error *****
IF %INVALID_DEVIF% == YES (
	echo 	Invalid device interface: %DEVIF%
	echo 	Accepted: [SDIO / GSPI]
)

IF %INVALID_CHIPID% == YES (
	echo 	Invalid chip_id: %CHIPID%
	echo 	Accepted: [8305 / 8381 / 8385 / 8686 / 8388]
)

IF %INVALID_RF% == YES (
	echo 	Invalid RF: %RF%
	echo 	Accepted: [8010 / 8015 / 8030 / 8031]
)

IF %INVALID_HOSTTYPE% == YES (
	echo 	Invalid device interface: %HOSTTYPE%
	echo 	Accepted: [PXA270 / X86 / S3C2443 ]
)

IF %INVALID_MODE% == YES (
	echo 	Invalid device interface: %MODE%
	echo 	Accepted: [B / BG / A / ABG]
)

rem	================================================================= 
:End


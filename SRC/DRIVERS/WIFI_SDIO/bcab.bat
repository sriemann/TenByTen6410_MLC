@echo off
rem	Copyright (c) Marvell Semiconductor, Inc.  All rights reserved. 
rem
rem	filename: bcab.bat
rem	Description: Utility to generate the .cab file
rem 
rem	================================================================= 

del SDIO8686.dll
copy .\wlan\obj\ARMV4I\%WINCEDEBUG%\SDIO8686.dll .
call sign SDIO8686.dll
call "E:\Program Files\Platform Builder for Windows Mobile\5.00\CEPB\BIN\cabwiz" SDIO8686.inf



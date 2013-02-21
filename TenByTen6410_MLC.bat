@REM
@REM Copyright (c) Microsoft Corporation.  All rights reserved.
@REM
@REM Use of this sample source code is subject to the terms of the 
@REM Software License Agreement (SLA) under which you licensed this software product.
@REM If you did not accept the terms of the license agreement, 
@REM you are not authorized to use this sample source code. 
@REM THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
@REM


@REM To support iROM NANDFlash boot
set BSP_IROMBOOT=1

@REM To support iROM SDMMC boot
set BSP_IROM_SDMMC_CH0_BOOT=
set BSP_IROM_SDMMC_CH1_BOOT=

set WINCEREL=1

set BSP_NODISPLAY=
set BSP_NOBACKLIGHT=1
set BSP_NOTOUCH=
set BSP_NOPWRBTN=1

set BSP_NONANDFS=1

@REM The BSP overrides the common SDBus2 driver. This will not take
@REM effect unless IMGSDBUS2 is set
set IMGSDBUS2=1

set BSP_NOHSMMC_CH0=
if /i "%BSP_IROM_SDMMC_CH0_BOOT%"=="1" set BSP_NOHSMMC_CH0=1
set BSP_NOHSMMC_CH1=
if /i "%BSP_IROM_SDMMC_CH1_BOOT%"=="1" set BSP_NOHSMMC_CH1=1
set BSP_HSMMC_CH1_8BIT=

@REM CF_ATAPI and KEYPAD cannot be enabled together due to hardware clash
@REM
set BSP_NOKEYPAD=1
set BSP_NOCFATAPI=1

set BSP_NOAUDIO=
set BSP_AUDIO_AC97=1

set BSP_NOD3DM=
set BSP_NOCAMERA=1

set BSP_NOMFC=
set BSP_NOJPEG=
set BSP_NOOES=
set BSP_NOCMM=
set BSP_NOUAO=

set BSP_NOSERIAL=
set BSP_NOUART0=
set BSP_NOUART1=
set BSP_NOUART2= 
set BSP_NOUART3= 1
set BSP_NOIRDA2=1
set BSP_NOIRDA3=1
@REM set BSP_BLUETOOTH_BUILTIN_UART=1

set BSP_NOI2C=
set BSP_NOSPI=

set BSP_NOUSBHCD=
set BSP_NO_LED=1
set BSP_NODM9000A1=
set BSP_NO_SDIO_WIFI=
set BSP_NOGPIO= 1
set BSP_NOEXTPOWERCTL =

@REM If you want to exclude USB Function driver in BSP. Set this variable
set BSP_NOUSBFN=
@REM This select default function driver
set BSP_USBFNCLASS=SERIAL
@REM set BSP_USBFNCLASS=MASS_STORAGE

@REM DVFS is not yet implemented.
set BSP_USEDVS=

@set BSP_DEBUGPORT=SERIAL_UART0
@REM set BSP_DEBUGPORT=SERIAL_UART1
@REM set BSP_DEBUGPORT=SERIAL_UART2
set BSP_DEBUGPORT=SERIAL_UART3


@REM set BSP_KITL=NONE
@REM set BSP_KITL=SERIAL_UART0
@REM set BSP_KITL=SERIAL_UART1
@REM set BSP_KITL=SERIAL_UART2
@REM set BSP_KITL=SERIAL_UART3
@REM set BSP_KITL=USBSERIAL

@REM For Hive Based Registry
set IMGHIVEREG=1
@REM if /i "%BSP_IROM_SDMMC_CH1_BOOT%"=="1" set IMGHIVEREG=

if /i "%IMGHIVEREG%"=="1" set PRJ_ENABLE_FSREGHIVE=1
if /i "%IMGHIVEREG%"=="1" set PRJ_ENABLE_REGFLUSH_THREAD=1

@REM Multipl-XIP using demand paging, BINFS must be turned on
set IMGMULTIXIP=

@REM Does not support
set IMGMULTIBIN=


@REM for using GDI Performance Utility, DispPerf
@REM there are some compatibility issue between CE6.0 R2 and previous version
@REM if you want to use DISPPERF in CE6.0 R2,
@REM build public components(GPE) also.
set DO_DISPPERF=


@REM Multimedia Samples
@REM DSHOWFILTERS must be on if use MFC Codec

set SAMPLES_DSHOWFILTERS=1
set SAMPLES_MFC=1
set SAMPLES_MFC_API=1
set SAMPLES_JPEG=1
set SAMPLES_HYBRIDDIVX=
set SAMPLES_CMM=1

@REM To test CE6.0 R3 NewFeature
@REM Adobe Flash Lite(SYSGEN_IE_FLASHLITE)
@REM Internet Explorer 6.0 XAML UI Sample Browser(SYSGEN_IESAMPLE_EXR)
@REM Internet Explorer 6.0 Tiling Engine(SYSGEN_IE_TILEENGINE)
@REM Gesture Support for Win32 Controls(SYSGEN_GESTUREANIMATION)
@REM Gesture Animation Support(SYSGEN_PHYSICENGINE)
@REM Silverlight for Windows Embedded(SYSGEN_XAML_RUNTIME)
@REM is provided in Catalog
@REM Silverlight for Windows Embedded Xamlperf.exe Sample Application
set SYSGEN_SAMPLEXAMLPERF=1

@REM Enable XAML Renderer HW Acceleration 
@REM Need to sysgen. Choose only one or no Renderer Plugin.
@REM Default Renderer Plugin is GDI Renderer
@REM DDRAW has no other prerequisite.
set BSP_XRPLUGIN_DDRAW=1
@REM Silverlight OPENGL Renderer Plugin requires Shaders.dll,
@REM Because 6410 has no runtime shader compiler.
@REM This option will build SAMPLES\SHADERS also.
@REM that is forked from PUBLIC\COMMON\OAK\XAMLRENDERPLUGIN\OPENGL\SHADERS
@REM You may need to sysgen again or build below.
@REM if Silverlight doesn't work correctly, ex) Shader Loading Error
@REM you can try to build SAMPLES\XRRENDEREROPENGL
@REM that's also forked from PUBLIC codes
set BSP_XRPLUGIN_OPENGL=

@REM ******** DDR Size Setting ******************
@REM BSP_DRAM128 and BSP_DRAM256 cannot be enabled  together due to hardware
@REM Must check Resistance R1 Xm1ADDR13 on  PCB ,then set BSP_DRAM256 to 1
@REM BSP_DRAM128  work based on S3C6410_APLL_CLK 667MHZ and SDRAM SIZE 128MB
@REM BSP_DRAM256  work based on S3C6410_APLL_CLK 667MHZ and SDRAM SIZE 256MB
set BSP_DRAM128=1
set BSP_DRAM256=2
@REM set BSP_TYPE=%BSP_DRAM128%
set BSP_TYPE=%BSP_DRAM256%
@REM ********************************************

@REM - To support PocketMory
call %_TARGETPLATROOT%\src\Whimory\wmrenv.bat


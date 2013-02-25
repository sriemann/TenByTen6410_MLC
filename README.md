This document list changes and ToDos for HITEG's TenByTen6410 MLC NAND WinCE 6R3 BSP



----------------------------- ToDo       -----------------------------------

1. mDDR RAMs can do 166MHz, we currently support 133MHz, only, to achive 166MHz, all depending Freq. have to be reviewed 
2. look if (1) is working, if we can also support 200MHz mDDRs....=>40% memory bandwidth improvement.
3. Apply changes for Rev. 2 of TenByTen6410
	a) no TV output, as connectors removed (SD TV is outdated)
	b) change Hirose 24pin keyboard connector to 24pin 2.0mm header, add I2C support there
	c) move 8pin connector (MINI2440v2 compatibility) to new location (TVOUT)
	d) support USB internal connector for USB hub chips ==> new layout
	e) EINT support for GPIODriver

----------------------------- 25.02.2013 -----------------------------------
1.	Rewrote GPIODriver to support GPIOs found on the TenByTen6410 GPIO connector
	Pin22 to PIN37 (16 pins) can be set/controlled as Input or output, 
	with or without pull up, pull down.
	Datums can be read/write pin by pin or as bulk ( see source code )
----------------------------- 19.02.2013 -----------------------------------

1.	Make new 10.4" display working, currently under VGA800x600 (7) works flawlessly without any glitches

----------------------------- 01.02.2013 -----------------------------------

1. SerialObject supports all four UARTs now. 
2. eBoot debug.c is independent from OALLIB/debug.c and defaults to UART0 (RS232)
3. updated the display driver, in LDI part we set the current for LCD pins to 2mA, which
   eliminates flickering, due to cable inductence. We tested with 250mm FFC to MegaDisplay7, no side effects visible.
4. In SVEDriver, we give correct LCD-type now, thus LDI lib is initialized correctly. LCD clock is generated from 133MHz source

5. project.reg has FTPD examples included. ( needs User authentification setup )

----------------------------- 19.05.2012 -----------------------------------
1. moved ExtPowerCtl LIB from driver section to common section
   as a Build from scratch can't satisfy dependencies in OAL section
2. added correct IOCTL codes ( via CTL_CODE MACRO ) to ExtPowerCtl_DRV
   epctl.h added containig these values
3. added GET functionality for each device in driver. 
4. created sample userspace application in Managed VB code ( later native ) to
   test driver and to serve as an example in how to use. 
5. Started with an GPIO driver, which is limited to the access able GPIO ports only
	- GPIO connector
	- LCD Connector
	- Keyboard connector
	- camera connector
	- MMC1 ( Wifi ) pins
	- serial ports
   SOme of those GPIOs can be configured as IRQ lines, looking into a way to make that
   work in WinCE6 and WinCE 7 alike.
----------------------- Eastern 2012 (stable) ---------------------
1. removed GPIO driver as Win7 needs different approach
2. created ExtPowerCtl
	a) Lib for the Driver and OALLIB
	b) Drv the Stream driver 
3. cleaned up LDI and Display driver sections
4. added proper ARGS section for display support from eBoot -> WinCE Kernel
 
------------------------- Jan/Feb 2012 ---------------------------------
1.Add Eboot select LCD type
  a.)modified \SRC\BOOTLOADER\EBOOT\main.c;
  b.)modified \SRC\DRIVERS\DISPLAY\DISPLAY_DRV\display_main.cpp;
  c.)modified \SRC\DRIVERS\DISPLAY\LDI_LIB\s3c6410_ldi.c
  
2.Add GPIO driver to control TV OUT and USB Host Power
  a.)\SRC\DRIVERS\GPIO
  b.)via IOCOTROL to control TV OUT and USB Host Power
  
3.Add Lan IC DM9000 driver
  a) \SRC\DRIVERS\DM9000A1
  b) modifed cs1--bank1 in\SRC\OAL\OALLIB\init.c ,EINT7  in intr.c
  
4.Add SDIO Wifi driver,mavell 8686
  a)\SRC\DRIVERS\MAVELL_WIFI
  
5.Modified touch driver to improve touch performance

6.According H/W modified HSMMC driver
  1)HSMMCCh1 use normal interrupt,EINT10


7.Add LED driver for led indicator 
 a.)\SRC\DRIVERS\Led 

8.Add EBOOT DM9000 downloading NK
 a,)modified \SMDK6410\SRC\BOOTLOADER\EBOOT\main.c and ether.c
 b,)modified \SMDK6410\SRC\BOOTLOADER\EBOOT\ sources
 
9.Setting RAM SIZE in  SMDK6410.bat
  a) 128M RAMSIZE set as beloow
  set BSP_TYPE=%BSP_DRAM128%
  @REM set BSP_TYPE=%BSP_DRAM256%
  
  b) 256M RAMSIZE set as beloow
  @REM set BSP_TYPE=%BSP_DRAM128%
  set BSP_TYPE=%BSP_DRAM256%
  
  c)modified \SMDK6410\sources.cmn
  d)modified \SMDK6410\SMDK6410.bat
  
10.256M an 128M RAM support 
   a)modified \SMDK6410\SRC\INC\image_cfg.h
   b)modified \SMDK6410\SRC\INC\image_cfg.inc
   c)modified \SMDK6410\SRC\INC\MemParam_mDDR.inc
   d)modified \SMDK6410\SRC\INC\oemaddrtab_cfg.inc
   e)modified \SMDK6410\SRC\OAL\OALLIB\init.c
   f)modified \SMDK6410\SRC\BOOTLOADER\STEPLDR\startup.s
   g)modified \SMDK6410\SRC\BOOTLOADER\EBOOT\startup.s
   

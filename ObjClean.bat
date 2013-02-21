@ rem filename: clean.bat
@ rem   - For use in CEBP 6.0 builds
@ rem   - Removes output file directories, which will be recreated during build
@ rem   - Also removes some individual file types
@ rem   - Must be run from platform\TENBYTEN6410_MLC directory
@
del /s /q build.*
del /s /q *.bif
rd /s /q cesysgen\files

rd /s /q target

del /s /q *.bak

rd /r /q SRC\BOOTLOADER\EBOOT\obj
rd /r /q SRC\BOOTLOADER\STEPLDR\obj
rd /r /q SRC\COMMON\ARGS\obj
rd /r /q SRC\COMMON\NANDFLASH\Fmd\obj
rd /r /q SRC\COMMON\PM\obj

rd /r /q SRC\DRIVERS\Backlight\obj

rd /r /q SRC\DRIVERS\CAMERA\CAMERA_PDD\obj
rd /r /q SRC\DRIVERS\CAMERA\DLL\obj
rd /r /q SRC\DRIVERS\CAMERA\MDD\obj
rd /r /q SRC\DRIVERS\CAMERA\OV9650_MODULE\obj
rd /r /q SRC\DRIVERS\CAMERA\S3C6410_CAMERA\obj

del /r /q SRC\DRIVERS\Can\obj

rd /r /q SRC\DRIVERS\CF_ATAPI\AtapiRomi\obj
rd /r /q SRC\DRIVERS\CF_ATAPI\COMMON\obj

rd /r /q SRC\DRIVERS\CMM\obj

rd /r /q SRC\DRIVERS\DISPLAY\s3c6410_disp_drv\obj
rd /r /q SRC\DRIVERS\DISPLAY\s3c6410_disp_lib\obj
rd /r /q SRC\DRIVERS\DISPLAY\s3c6410_g2d_lib\obj
rd /r /q SRC\DRIVERS\DISPLAY\s3c6410_ldi_lib\obj
rd /r /q SRC\DRIVERS\DISPLAY\s3c6410_post_lib\obj
rd /r /q SRC\DRIVERS\DISPLAY\s3c6410_rotator_lib\obj
rd /r /q SRC\DRIVERS\DISPLAY\s3c6410_tv_encoder_lib\obj
rd /r /q SRC\DRIVERS\DISPLAY\s3c6410_tv_scaler_lib\obj
rd /r /q SRC\DRIVERS\DISPLAY\s3c6410_video_drv\obj

rd /r /q SRC\DRIVERS\DM9000A1\obj
rd /r /q SRC\DRIVERS\DM9000A2\obj

rd /r /q SRC\DRIVERS\DMA\s3c6410_dma_lib\obj

rd /r /q SRC\DRIVERS\DrvLib\obj

rd /r /q SRC\DRIVERS\HSMMC\HSMMCCh0\s3c6410_hsmmc_drv\obj
rd /r /q SRC\DRIVERS\HSMMC\HSMMCCh0\s3c6410_hsmmc_lib\obj
rd /r /q SRC\DRIVERS\HSMMC\HSMMCCh1\s3c6410_hsmmc_drv\obj
rd /r /q SRC\DRIVERS\HSMMC\HSMMCCh1\s3c6410_hsmmc_lib\obj

rd /r /q SRC\DRIVERS\JPEG\obj

rd /r /q SRC\DRIVERS\KEYBD\kbds3c6410\obj
rd /r /q SRC\DRIVERS\KEYBD\keypad\obj
rd /r /q SRC\DRIVERS\KEYBD\Matrix_0409\obj
rd /r /q SRC\DRIVERS\KEYBD\Pddlist\obj

rd /r /q SRC\DRIVERS\Led\obj

rd /r /q SRC\DRIVERS\MFC\mfc_dd_if_layer\wince\src\obj
rd /r /q SRC\DRIVERS\MFC\mfc_os_dep_layer\src\obj
rd /r /q SRC\DRIVERS\MFC\mfc_os_indep_layer\src\obj

rd /r /q SRC\DRIVERS\NANDFLASH\obj

rd /r /q SRC\DRIVERS\OTG\Device\obj

rd /r /q SRC\DRIVERS\POWERBUTTON\obj

rd /r /q SRC\DRIVERS\POWERCONTROL\obj

rd /r /q SRC\DRIVERS\SERIAL\obj

rd /r /q SRC\DRIVERS\TOUCH\obj

rd /r /q SRC\DRIVERS\UAO\obj

rd /r /q SRC\DRIVERS\SPI\obj
rd /r /q SRC\DRIVERS\SPI__\obj

rd /r /q SRC\DRIVERS\USB\Hcd\obj

rd /r /q SRC\DRIVERS\IIC\MDD\obj
rd /r /q SRC\DRIVERS\IIC\s3c6410_iic_lib\obj
rd /r /q SRC\DRIVERS\IIC__\MDD\obj
rd /r /q SRC\DRIVERS\IIC__\s3c6410_iic_lib\obj

rd /r /q SRC\DRIVERS\WAVEDEV\s3c6410_ac97_drv\obj
rd /r /q SRC\DRIVERS\WAVEDEV\s3c6410_ac97_lib\obj
rd /r /q SRC\DRIVERS\WAVEDEV\s3c6410_iis_drv\obj
rd /r /q SRC\DRIVERS\WAVEDEV\s3c6410_iis_lib\obj

rd /s /q SAMPLES\CMM\Src\obj
rd /s /q SAMPLES\DSHOWFILTERS\AACDecoderFilter\src\obj
rd /s /q SAMPLES\DSHOWFILTERS\FrameExtractFilter\obj
rd /s /q SAMPLES\DSHOWFILTERS\MFCDecoderFilter\obj
rd /s /q SAMPLES\DSHOWFILTERS\MFCLib\src\obj
rd /s /q SAMPLES\DSHOWFILTERS\MMFrameExtractLib\obj
rd /s /q SAMPLES\DSHOWFILTERS\MP4testcode\obj
rd /s /q SAMPLES\DSHOWFILTERS\mpeg4ip_mp4v2\src\obj
rd /s /q SAMPLES\DSHOWFILTERS\SsapMp4Parser\obj
rd /s /q SAMPLES\HYBRIDDIVX\Test\Src\obj
rd /s /q SAMPLES\JPEG\JPG_API\src\obj
rd /s /q SAMPLES\JPEG\test\src\obj
rd /s /q SAMPLES\MFC\MFC_DecodeDemo\MfcDemo\obj
rd /s /q SAMPLES\MFC\MFC_DecodeDemo\MFCLib\src\obj
rd /s /q SAMPLES\MFC\MFC_DecodeDemo\MMFrameExtractLib\obj
rd /s /q SAMPLES\MFC\VFP_TEST\VFP_ADDTEST\obj
rd /s /q SAMPLES\MFC\VFP_TEST\VFP_DIVTEST\obj
rd /s /q SAMPLES\MFC\VFP_TEST\VFP_MULTEST\obj
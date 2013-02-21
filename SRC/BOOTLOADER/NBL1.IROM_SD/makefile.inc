!IF 0


Module Name:

    makefile.inc

!ENDIF
#
#
#
stepldr_romimage:
!IF "$(NOLINK)" == ""
    romimage $(ROMIMAGE_FLAGS) stepldr_irom_sd.bib 
	copy /y/b $(_PLATFORMROOT)\$(_TGTPLAT)\target\$(_TGTCPU)\$(WINCEDEBUG)\eboot.nb0 + $(_PLATFORMROOT)\$(_TGTPLAT)\target\$(_TGTCPU)\$(WINCEDEBUG)\STEPLDR_IROM_SD.nb0 $(_PLATFORMROOT)\$(_TGTPLAT)\target\$(_TGTCPU)\$(WINCEDEBUG)\IROM_SD_EBOOT.nb0
!IF "$(WINCEREL)"=="1"
    copy $(_PLATFORMROOT)\$(_TGTPLAT)\target\$(_TGTCPU)\$(WINCEDEBUG)\IROM_SD_EBOOT.* $(_FLATRELEASEDIR)
    copy $(_PLATFORMROOT)\$(_TGTPLAT)\target\$(_TGTCPU)\$(WINCEDEBUG)\STEPLDR_IROM_SD.* $(_FLATRELEASEDIR)
!ENDIF
!ENDIF
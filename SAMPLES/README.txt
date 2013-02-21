README about Samples
====================

1. DShowFilters
---------------

- Samples for DShowFilters are enabled by 'set SAMPLES_DSHOWFILTERS=1' in SMDK6410.bat file.
- When enabled, the DirectShow filters are included in Windows CE image.
- List of filters
  (1) FrameExtractFilter (source filter)
      It is reading the raw video stream files (Files with extention, m4v, 263, 264, rcv)
  (2) SsapMp4Parser (source filter)
      It is reading the mp4 files (Files with extention, mp4, mov, 3g2)
      mp4 file can contain video streams of MPEG4, H.263, H.264
      and audio streams of AAC, AMR-NB(AMR-NB will be available.)
  (3) MFCDecoderFilter (transform filter)
      It is MFC(Multi-format Codec; S3C6410 HW video decoder) decoder filter.
      It is connected to the source filter (FrameExtractFilter or SsapMp4Parser).
  (4) AACDecoderFilter (transform filter)
      It is S/W AAC audio decoder filter.
      It is connected to the SsapMp4Parser filter.
      If the mp4 file contains the AAC audio, then you can hear the sound.



2. Jpeg
-------

- Samples for HW Jpeg encoder/decoder
- It is enabled by 'set SAMPLES_JPEG=1' in SMDK6410.bat file.
- The sample files are NOT included in Windows CE image.
- How to run
  (1) Copy the "samples\Jpeg\test\src\obj\ARMV4I\retail\jpg_test.exe" and
      whole testVectors folder and "fname_dec.txt", "fname_enc.txt"
      to device's root directory. (must be copied to "Root" directory.)
  (2) Execute the jpg_test.exe.



3. Mfc
------

- Samples for HW Mfc decoder
- It is enabled by 'set SAMPLES_MFC=1' in SMDK6410.bat file.
- The sample files are NOT included in Windows CE image.
- How to run
  (1) Copy the "samples\Mfc\MFC_DecodeDemo\MfcDemo\obj\ARMV4I\retail\MFC_Demo.exe"
      to device's directory. (Any directory is ok.)
  (2) Copy the video raw streams (Files with extention, m4v, 263, 264, rcv)
      to device's "\My Documents" directory.
  (3) Execute the MFC_Demo.exe.

4. CMM
------

- Samples for CMM(Codec Memory Menagement) driver
- It is enabled by 'set SAMPLES_CMM=1' in SMDK6410.bat file.
- The sample files are NOT included in Windows CE image.
- How to run
  (1) Copy the "samples\Cmm\Src\obj\ARMV4I\retail\cmm_test.exe"
      to device's directory. (Any directory is ok.)
  (2) Copy the YUV raw streams, "samples\Cmm\testvector\test.yuv"
      to device's "\" directory.
  (3) Execute the cmm_test.exe.
  (4) For details, refer "samples\Doc\S3C6410_WM6.1_CMM_UsersManual_REV1.1_20080723.doc"


5. HYBRIDDIVX
------

- Samples for DivX decoder
- It is enabled by 'set SAMPLES_HYBRIDDIVX=1' in SMDK6410.bat file.
- The sample files are NOT included in Windows CE image.
- How to run
  (1) Copy the "samples\HybridDivX\Test\Src\obj\ARMV4I\retail\HybridDivxTest.exe"
      to device's directory. (Any directory is ok.)
  (2) List up the test file in "samples\HybridDivX\Test\DivxFileList.txt"
  (3) Copy the DivxFileList.txt and video streams (Files with extention, .avi)
      to device's "\" directory.
  (4) Execute the HybridDivxTest.exe.
  (5) For details, refer "samples\Doc\S3C6410_HybridDivxDecoder_API_REV0.4_20080716.doc"

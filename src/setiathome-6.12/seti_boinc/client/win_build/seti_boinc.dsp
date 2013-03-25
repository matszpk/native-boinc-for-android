# Microsoft Developer Studio Project File - Name="seti_boinc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=seti_boinc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "seti_boinc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "seti_boinc.mak" CFG="seti_boinc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "seti_boinc - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "seti_boinc - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "seti_boinc - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "." /I "../../../boinc/api" /I "../../../boinc/lib" /I ".." /I "glut" /I "../../../boinc/image_libs" /I "../../../boinc/jpeglib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "BOINC_APP_GRAPHICS" /D "_MBCS" /D "CLIENT" /FR /FD /c /Tp
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 glut32.lib /nologo /subsystem:windows /machine:I386 /libpath:"glut"
# SUBTRACT LINK32 /nodefaultlib

!ELSEIF  "$(CFG)" == "seti_boinc - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "." /I "../../../boinc/api" /I "../../../boinc/client/win" /I "../../../boinc/lib" /I ".." /I "glut" /I "../../../boinc/image_libs" /I "../../../boinc/jpeglib" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "CLIENT" /D "BOINC_APP_GRAPHICS" /FR /FD /GZ /TP /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 glut32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"Debug/setiathome_2.18_windows_intelx86.exe" /pdbtype:sept /libpath:"glut"
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "seti_boinc - Win32 Release"
# Name "seti_boinc - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\amd64AnalyzeFuncs.cpp
# End Source File
# Begin Source File

SOURCE=..\amd64fft8g.cpp
# End Source File
# Begin Source File

SOURCE=..\analyzeFuncs.cpp
# End Source File
# Begin Source File

SOURCE=..\analyzePoT.cpp
# End Source File
# Begin Source File

SOURCE=..\analyzeReport.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\lib\app_ipc.C
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\image_libs\bmplib.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\api\boinc_api.C
# End Source File
# Begin Source File

SOURCE=..\chirpfft.cpp
# End Source File
# Begin Source File

SOURCE=..\fft8g.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\lib\filesys.C
# End Source File
# Begin Source File

SOURCE=..\gaussfit.cpp
# End Source File
# Begin Source File

SOURCE=..\gdata.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\api\graphics_api.C
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\api\graphics_data.C
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\api\gutil.C
# End Source File
# Begin Source File

SOURCE=..\lcgamm.cpp
# End Source File
# Begin Source File

SOURCE=..\main.cpp
# End Source File
# Begin Source File

SOURCE=..\malloc_a.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\api\mfile.C
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\lib\parse.C
# End Source File
# Begin Source File

SOURCE=..\pulsefind.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\api\reduce.C
# End Source File
# Begin Source File

SOURCE=..\s_util.cpp
# End Source File
# Begin Source File

SOURCE=..\sah_gfx.cpp
# End Source File
# Begin Source File

SOURCE=..\sah_gfx_base.cpp
# End Source File
# Begin Source File

SOURCE=..\..\db\schema_master.cpp
# End Source File
# Begin Source File

SOURCE=..\seti.cpp
# End Source File
# Begin Source File

SOURCE=..\seti_header.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\lib\shmem.C
# End Source File
# Begin Source File

SOURCE=..\spike.cpp
# End Source File
# Begin Source File

SOURCE=..\..\db\sqlblob.cpp
# End Source File
# Begin Source File

SOURCE=..\..\db\sqlrow.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\image_libs\tgalib.cpp
# End Source File
# Begin Source File

SOURCE=..\timecvt.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\lib\util.C
# End Source File
# Begin Source File

SOURCE=..\version.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\api\windows_opengl.C
# End Source File
# Begin Source File

SOURCE=..\worker.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\lib\xml_util.C
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\analyze.h
# End Source File
# Begin Source File

SOURCE=..\analyzeFuncs.h
# End Source File
# Begin Source File

SOURCE=..\analyzePoT.h
# End Source File
# Begin Source File

SOURCE=..\analyzeReport.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\lib\app_ipc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\image_libs\bmplib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\api\boinc_api.h
# End Source File
# Begin Source File

SOURCE=..\chirpfft.h
# End Source File
# Begin Source File

SOURCE=.\sah_config.h
# End Source File
# Begin Source File

SOURCE=..\..\db\db_table.h
# End Source File
# Begin Source File

SOURCE=..\fft8g.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\lib\filesys.h
# End Source File
# Begin Source File

SOURCE=..\gaussfit.h
# End Source File
# Begin Source File

SOURCE=..\gdata.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\api\graphics_api.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\api\graphics_data.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\api\gutil.h
# End Source File
# Begin Source File

SOURCE=..\lcgamm.h
# End Source File
# Begin Source File

SOURCE=..\malloc_a.h
# End Source File
# Begin Source File

SOURCE=..\pulsefind.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\api\reduce.h
# End Source File
# Begin Source File

SOURCE=..\s_util.h
# End Source File
# Begin Source File

SOURCE=..\sah_gfx.h
# End Source File
# Begin Source File

SOURCE=..\sah_gfx_base.h
# End Source File
# Begin Source File

SOURCE=..\..\db\schema_master.h
# End Source File
# Begin Source File

SOURCE=..\seti.h
# End Source File
# Begin Source File

SOURCE=..\seti_header.h
# End Source File
# Begin Source File

SOURCE=..\spike.h
# End Source File
# Begin Source File

SOURCE=..\..\db\sqlblob.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\lib\std_fixes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\image_libs\tgalib.h
# End Source File
# Begin Source File

SOURCE=..\timecvt.h
# End Source File
# Begin Source File

SOURCE=..\version.h
# End Source File
# Begin Source File

SOURCE="..\win-sah_config.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\client\win\win_idle_tracker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\client\win\win_util.h
# End Source File
# Begin Source File

SOURCE=..\worker.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\lib\xml_util.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "jpeglib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\cderror.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\cdjpeg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jcapimin.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jcapistd.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jccoefct.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jccolor.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jcdctmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jchuff.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jchuff.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jcinit.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jcmainct.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jcmarker.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jcmaster.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jcomapi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jconfig.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jcparam.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jcphuff.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jcprepct.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jcsample.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jctrans.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdapimin.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdapistd.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdatadst.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdatasrc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdcoefct.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdcolor.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdct.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jddctmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdhuff.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdhuff.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdinput.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdmainct.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdmarker.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdmaster.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdmerge.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdphuff.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdpostct.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdsample.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jdtrans.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jerror.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jerror.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jfdctflt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jfdctfst.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jfdctint.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jidctflt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jidctfst.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jidctint.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jidctred.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jinclude.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jmemmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jmemnobs.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jmemsys.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jpegint.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jpeglib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jquant1.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jquant2.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jutils.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\jversion.h
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\rdbmp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\rdcolmap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\rdgif.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\rdppm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\rdrle.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\rdswitch.c
# End Source File
# Begin Source File

SOURCE=..\..\..\boinc\jpeglib\rdtarga.c
# End Source File
# End Group
# End Target
# End Project

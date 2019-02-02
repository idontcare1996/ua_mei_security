!include makefile.inc

TOPDIR = ..\..

TARGET                  = pteidlibopensc.dll

HEADERS			= opensc.h pkcs15.h emv.h \
			  errors.h types.h \
			  cardctl.h asn1.h log.h scgui.h pteidpinpad1.h part10.h

HEADERSDIR		= $(TOPDIR)\src\include\opensc

OBJECTS			= \
			  sc.obj ctx.obj module.obj asn1.obj log.obj base64.obj padding.obj \
			  errors.obj sec.obj card.obj iso7816.obj dir.obj ctbcs.obj \
			  portability.obj \
			  pkcs15.obj pkcs15-cert.obj pkcs15-data.obj pkcs15-pin.obj \
			  pkcs15-prkey.obj pkcs15-pubkey.obj pkcs15-sec.obj \
			  pkcs15-wrap.obj pkcs15-algo.obj \
			  pkcs15-cache.obj reader-pcsc.obj $(TOPDIR)\win32\version.res \
			  card-pteid.obj card-ias.obj

all: install-headers $(TARGET)

!INCLUDE $(TOPDIR)\win32\Make.rules.mak


$(TARGET): $(OBJECTS) $(TARGET2)
	perl $(TOPDIR)\win32\makedef.pl $*.def $* $(OBJECTS)
	link $(LINKFLAGS) /dll /def:$*.def /implib:$*.lib /out:$(TARGET) $(OBJECTS) \
	..\scconf\scconf.lib ..\scdl\scdl.lib winscard.lib advapi32.lib gdi32.lib Ws2_32.lib \
	user32.lib shell32.lib ole32.lib $(TOPDIR)\..\openssl-0.9.7l\out32dll\ptlibeay32_0_9_7l.lib
	$(_VC_MANIFEST_EMBED_DLL)
	
!include makefile.targ.inc
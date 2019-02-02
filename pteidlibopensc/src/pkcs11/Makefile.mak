!include makefile.inc

TOPDIR = ..\..

HEADERS			= pkcs11.h scrandom.h
HEADERSDIRFROM2		= rsaref

HEADERSDIR		= $(TOPDIR)\src\include\opensc
HEADERSDIR2		= $(TOPDIR)\src\include\opensc\rsaref

TARGET			= pteidpkcs11.dll
TARGET2			= libpkcs11.lib

OBJECTS			= pkcs11-global.obj pkcs11-session.obj pkcs11-object.obj misc.obj slot.obj \
			  secretkey.obj framework-pkcs15.obj framework-pkcs15init.obj mechanism.obj \
			  openssl.obj debug.obj scrandom.obj $(TOPDIR)\win32\version.res
OBJECTS2		= libpkcs11.obj
OBJECTS3		= pkcs11-spy.obj pkcs11-display.obj libpkcs11.obj

all: install-headers install-headers-dir $(TARGET) $(TARGET2)

!INCLUDE $(TOPDIR)\win32\Make.rules.mak

$(TARGET): $(OBJECTS)
	link $(LINKFLAGS) /dll /out:$(TARGET) $(OBJECTS) ..\libopensc\pteidlibopensc.lib \
	..\scconf\scconf.lib winscard.lib $(TOPDIR)\..\openssl-0.9.7l\out32dll\ptlibeay32_0_9_7l.lib \
	gdi32.lib advapi32.lib
	$(_VC_MANIFEST_EMBED_DLL)

$(TARGET2): $(OBJECTS2)
	lib /nologo /machine:ix86 /out:$(TARGET2) $(OBJECTS2) ..\scdl\scdl.lib

!include makefile.targ.inc
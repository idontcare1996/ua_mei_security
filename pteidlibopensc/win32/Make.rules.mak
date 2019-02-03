COPTS = /Zi /MD /nologo /DHAVE_OPENSSL /DHAVE_CONFIG_H /DVERSION=\"1.20.0\" /I$(TOPDIR)\src\include /I$(TOPDIR)\..\openssl-0.9.7l\include /D_WIN32_WINNT=0x0400 /DHAVE_GUI /DBELPIC_PIN_PAD
LINKFLAGS = /DEBUG /NOLOGO /INCREMENTAL:NO /MACHINE:IX86


install-headers:
	@for %i in ( $(HEADERS) ) do \
		@xcopy /d /q /y %i $(HEADERSDIR) > nul

install-headers-dir:
	@for %i in ( $(HEADERSDIRFROM2) ) do \
		@xcopy /d /q /y %i\*.h $(HEADERSDIR2)\*.h > nul

.c.obj::
	cl $(COPTS) /c $<

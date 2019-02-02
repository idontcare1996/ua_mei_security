SUBDIRS = ..\win32 include common scconf scdl libopensc pkcs11 tools

all::

all depend install clean::
	@for %i in ( $(SUBDIRS) ) do \
        	@cmd /c "cd %i && $(MAKE) /nologo /f Makefile.mak $@"
        	
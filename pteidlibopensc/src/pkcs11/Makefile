INCLUDE_DIR=../include/opensc

include ../../../versions.linux

CC=     gcc 
CFLAGS= -DHAVE_GUI -DHAVE_CONFIG_H -fPIC -DPIC -DHAVE_PCSC -DHAVE_OPENSSL -fno-strict-aliasing -O2 -MD -MP  -c -fno-common -I. -I../.. -I../include
LFLAGS= -shared -lpcsclite -lcrypto

OBJS=	../scconf/parse.o ../scconf/write.o ../scconf/scconf.o ../scconf/lex-parse.o \
	../scdl/scdl.o \
	debug.o scrandom.o openssl.o \
	libpkcs11.o misc.o secretkey.o mechanism.o pkcs11-display.o slot.o \
	pkcs11-object.o pkcs11-session.o pkcs11-global.o \
	framework-pkcs15.o framework-pkcs15init.o

LIB_DIR=$(INSTALL_DIR)/lib

INSTALL_HEADERS=scrandom.h pkcs11.h
HEADERS=$(INSTALL_HEADERS) pkcs11-display.h sc-pkcs11.h
INCLUDE_DIR=../include/opensc
RSA_INC_DIR=$(INCLUDE_DIR)/rsaref

all: copy_headers $(PKCS11_NAME_x_y_z_dylib) make_bundle

clean:
	- rm *.o
	- rm *.d
	- rm *.dylib

install:
	cp -f $(PKCS11_NAME_x_y_z_dylib) $(LIB_DIR)/
	ln -f -s $(LIB_DIR)/$(PKCS11_NAME_x_y_z_dylib) $(LIB_DIR)/$(PKCS11_NAME_x_y_dylib)
	ln -f -s $(LIB_DIR)/$(PKCS11_NAME_x_y_dylib) $(LIB_DIR)/$(PKCS11_NAME_x_dylib)
	ln -f -s $(LIB_DIR)/$(PKCS11_NAME_x_dylib) $(LIB_DIR)/$(PKCS11_NAME_dylib)
	- rm -rf $(LIB_DIR)/pteidpkcs11.bundle
	cp -r pteidpkcs11.bundle $(LIB_DIR)/pteidpkcs11.bundle
	ln -f -s $(LIB_DIR)/$(PKCS11_NAME_x_dylib) $(LIB_DIR)/pteidpkcs11.bundle/Contents/MacOS/$(PKCS11_NAME_dylib)

uninstall:
	test $(LIB_DIR)/$(PKCS11_NAME_dylib) && rm -f $(LIB_DIR)/$(PKCS11_NAME_dylib)
	test $(LIB_DIR)/$(PKCS11_NAME_x_dylib) && rm -f $(LIB_DIR)/$(PKCS11_NAME_x_dylib)
	test $(LIB_DIR)/$(PKCS11_NAME_x_y_dylib) && rm -f $(LIB_DIR)/$(PKCS11_NAME_x_y_dylib)
	test $(LIB_DIR)/$(PKCS11_NAME_x_y_z_dylib) && rm -f $(LIB_DIR)/$(PKCS11_NAME_x_y_z_dylib)

$(PKCS11_NAME_x_y_z_dylib): $(OBJS) ../libopensc/$(LIBOPENSC_NAME_x_y_z_dylib)
	$(CC) $(LFLAGS) $(OBJS) ../libopensc/$(LIBOPENSC_NAME_x_y_z_dylib) -Wl,-soname,$(PKCS11_NAME_x_dylib) -o $(PKCS11_NAME_x_y_z_dylib)
	ln -f -s $(PKCS11_NAME_x_y_z_dylib) $(PKCS11_NAME_x_y_dylib)
	ln -f -s $(PKCS11_NAME_x_y_dylib) $(PKCS11_NAME_x_dylib)
	ln -f -s $(PKCS11_NAME_x_dylib) $(PKCS11_NAME_dylib)

make_bundle:
	- rm -rf pteidpkcs11.bundle
	cp -r ../../../mac/pteidpkcs11.bundle pteidpkcs11.bundle
	sed "s;PTEIDPKCS11_NAME_DYLIB;$(PKCS11_NAME_dylib);g" pteidpkcs11.bundle/Contents/Info.plist > tmp
	mv -f tmp pteidpkcs11.bundle/Contents/Info.plist

$(OBJS): $(HEADERS)

copy_headers:
	- cp -f scrandom.h $(INCLUDE_DIR)
	- cp -f pkcs11.h $(INCLUDE_DIR)
	#cp rsaref/pkcs11.h $(RSA_INC_DIR)
	#cp rsaref/pkcs11t.h $(RSA_INC_DIR)
	#cp rsaref/pkcs11f.h $(RSA_INC_DIR)
	#cp rsaref/unix.h $(RSA_INC_DIR)
	- cp -f rsaref/pkcs11.h $(RSA_INC_DIR)
	- cp -f rsaref/pkcs11t.h $(RSA_INC_DIR)
	- cp -f rsaref/pkcs11f.h $(RSA_INC_DIR)
	- cp -f rsaref/unix.h $(RSA_INC_DIR)


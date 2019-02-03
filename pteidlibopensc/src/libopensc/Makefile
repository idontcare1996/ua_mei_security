INCLUDE_DIR=../include/opensc

include ../../../versions.linux

CC=     gcc  
CFLAGS= -DHAVE_GUI -DHAVE_CONFIG_H -DPIC -DHAVE_PCSC -fPIC -DHAVE_OPENSSL -fno-strict-aliasing -O2 -MD -MP  -c -fno-common -I. -I../.. -I../include -I/usr/include/PCSC
LFLAGS= -shared -lpcsclite -lcrypto  

OBJS=	../scconf/parse.o ../scconf/write.o ../scconf/scconf.o ../scconf/lex-parse.o \
	../scdl/scdl.o \
	asn1.o base64.o log.o errors.o portability.o \
	card.o module.o ctx.o sc.o sec.o padding.o \
	ctbcs.o reader-pcsc.o \
	iso7816.o emv.o card-pteid.o card-ias.o \
	pkcs15.o pkcs15-sec.o pkcs15-wrap.o pkcs15-algo.o pkcs15-cache.o dir.o \
	pkcs15-prkey.o pkcs15-cert.o pkcs15-pubkey.o pkcs15-pin.o pkcs15-data.o

LIB_DIR=$(INSTALL_DIR)/lib

INSTALL_HEADERS=opensc.h errors.h cardctl.h log.h pkcs15.h types.h asn1.h
HEADERS=$(INSTALL_HEADERS) internal.h ctbcs.h resource.h scr.h scgui.h emv.h
INCLUDE_DIR=../include/opensc

all: copy_headers $(LIBOPENSC_NAME_x_y_z_dylib)

clean:
	- rm *.o
	- rm *.d
	- rm *.so.*


install:
	cp -f $(LIBOPENSC_NAME_x_y_z_dylib) $(LIB_DIR)/
	ln -f -s $(LIB_DIR)/$(LIBOPENSC_NAME_x_y_z_dylib) $(LIB_DIR)/$(LIBOPENSC_NAME_x_y_dylib)
	ln -f -s $(LIB_DIR)/$(LIBOPENSC_NAME_x_y_dylib) $(LIB_DIR)/$(LIBOPENSC_NAME_x_dylib)
	ln -f -s $(LIB_DIR)/$(LIBOPENSC_NAME_x_dylib) $(LIB_DIR)/$(LIBOPENSC_NAME_dylib)

uninstall:
	test $(LIB_DIR)/$(LIBOPENSC_NAME_dylib) && rm -f $(LIB_DIR)/$(LIBOPENSC_NAME_dylib)	
	test $(LIB_DIR)/$(LIBOPENSC_NAME_x_dylib) && rm -f $(LIB_DIR)/$(LIBOPENSC_NAME_x_dylib)	
	test $(LIB_DIR)/$(LIBOPENSC_NAME_x_y_dylib) && rm -f $(LIB_DIR)/$(LIBOPENSC_NAME_x_y_dylib)	
	test $(LIBOPENSC_NAME_x_y_z_dylib) && rm -f $(LIB_DIR)/$(LIBOPENSC_NAME_x_y_z_dylib)	

$(LIBOPENSC_NAME_x_y_z_dylib): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -Wl,-soname,$(LIBOPENSC_NAME_x_dylib) -o $(LIBOPENSC_NAME_x_y_z_dylib) 
	ln -f -s $(LIBOPENSC_NAME_x_y_z_dylib) $(LIBOPENSC_NAME_x_y_dylib)
	ln -f -s $(LIBOPENSC_NAME_x_y_dylib) $(LIBOPENSC_NAME_x_dylib)
	ln -f -s $(LIBOPENSC_NAME_x_dylib) $(LIBOPENSC_NAME_dylib)

$(OBJS): $(HEADERS)

copy_headers:
	cp -f opensc.h $(INCLUDE_DIR)
	cp -f errors.h $(INCLUDE_DIR)
	cp -f cardctl.h $(INCLUDE_DIR)
	cp -f log.h $(INCLUDE_DIR)
	cp -f pkcs15.h $(INCLUDE_DIR)
	cp -f types.h $(INCLUDE_DIR)
	cp -f asn1.h $(INCLUDE_DIR)
	cp -f scgui.h $(INCLUDE_DIR)

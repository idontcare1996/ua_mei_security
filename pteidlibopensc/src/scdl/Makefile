CC=gcc  
CFLAGS=-DHAVE_CONFIG_H -fPIC -DPIC -DHAVE_PCSC -DHAVE_OPENSSL -fno-strict-aliasing -O2 -MD -MP  -c -fno-common -I. -I../.. -I../../src/include

OBJS = scdl.o

INSTALL_HEADERS=scdl.h
HEADERS=$(INSTALL_HEADERS)
INCLUDE_DIR=../include/opensc

all: $(OBJS) copy_headers

clean:
	- rm *.o
	- rm *.d

$(OBJS): $(HEADERS)

copy_headers:
	cp -f scdl.h $(INCLUDE_DIR)

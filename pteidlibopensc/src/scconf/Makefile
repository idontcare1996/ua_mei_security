CC=gcc 
CFLAGS=-DYY_NEVER_INTERACTIVE -DHAVE_CONFIG_H -fPIC -DPIC -DHAVE_PCSC -DHAVE_OPENSSL -fno-strict-aliasing -O2 -MD -MP  -c -fno-common -I. -I../.. -I../../src/include

OBJS = parse.o scconf.o test.o lex-parse.o

INSTALL_HEADERS=scconf.h
HEADERS=$(INSTALL_HEADERS) internal.h
INCLUDE_DIR=../include/opensc

all: $(OBJS) copy_headers

clean:
	- rm *.o
	- rm *.d

$(OBJS): $(HEADERS)

copy_headers:
	- cp -f scconf.h $(INCLUDE_DIR)

CC=gcc
CFLAGS=-I./include -fPIC -std=c11
DEPS = libopenjpegextra.h 

all: libopenjpegextra

%.o: %.c $(DEPS)
	$(CC) -v -c -o $@ $< $(CFLAGS) -I$(includedir) -L$(libdir) -lopenjp2

TYPE=-shared
dlext=so
libdir=/usr/local/lib
includedir=/usr/local/include/openjpeg-2.5
libopenjpegextra: libopenjpegextra.o
	$(CC) $(TYPE) -v $@.o -o $@.$(dlext) -I$(includedir) -L$(libdir) -lopenjp2

# ------------
# To install the shared objects in their respective locations
libdir1=/usr/lib
includedir1=/usr/include
install: libopenjpegextra.$(dlext)
	install -Dvm 755 libopenjpegextra.$(dlext) $(libdir1)/libopenjpegextra.$(dlext)
	install -Dvm 644 libopenjpegextra.h $(includedir1)/libopenjpegextra.h


# make clean : To remove .o,.a,.so files in the current directory
clean :
	-rm *.o *.so 
	echo Cleared 

CC=gcc
CFLAGS=-I./include -fPIC -std=c11
DEPS = libopenjpegextra.h 

all: libopenjpegextra

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -L$(libdir) -lopenjp2

TYPE=-shared

libopenjpegextra: libopenjpegextra.o
	$(CC) $(TYPE) -o $@.$(dlext) -L$(libdir) -lopenjp2

# ------------
# To install the shared objects in their respective locations

install: libopenjpegextra.$(dlext)
	install -Dvm 755 libopenjpegextra.$(dlext) $(libdir)/libopenjpegextra.$(dlext)
	install -Dvm 644 include/libopenjpegextra.h $(includedir)/libopenjpegextra.h


# make clean : To remove .o,.a,.so files in the current directory
clean :
	-rm *.o *.so 
	echo Cleared 

# libopenjpegextra

sudo make
sudo make install
gcc test.c -o test.o -I/usr/local/include/openjpeg-2.5 -L/usr/local/lib -lopenjp2 -lopenjpegextra
./test.o
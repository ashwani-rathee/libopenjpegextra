#include <stdio.h>
#include <opj_config.h>
#include <openjpeg.h>
#include <libopenjpegextra.h>
#include <stdlib.h>
#include <string.h>


int main()
{
    opj_image_t *image;

    // Decoding success
    // image = decode("sample1.jp2");
    image = decode("sample.j2k");
    // // opj_image_t to rgb buffer data
    // rgb buf = imagetorgbbuffer(image);

    // // rgb buffer to opj_image_t format
    // opj_image_t *image1 = rgbbuffertoimage(image, buf); // I use image to get the original settings

    // opj_image_t image to jp2 file
    encode("sample1.jp2", image);

    return 0;
}
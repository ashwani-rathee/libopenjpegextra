#include <stdio.h>
#include "openjpeg-2.4/opj_config.h"
#include "openjpeg-2.4/openjpeg.h"
#include <stdlib.h>
#include <string.h>

struct rgbdata
{
    OPJ_INT32 *r;
    OPJ_INT32 *g;
    OPJ_INT32 *b;
};

typedef struct rgbdata rgb;

extern opj_image_t *convert_gray_to_rgb(opj_image_t *original);

extern int encode(char *filename, opj_image_t *image);
extern opj_image_t *decode(char *filename);
extern rgb imagetorgbbuffer(opj_image_t *image);
extern opj_image_t *rgbbuffertoimage(opj_image_t *original, rgb buf);

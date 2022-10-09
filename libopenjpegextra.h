#include <stdio.h>
#include <opj_config.h>
#include <openjpeg.h>
#include <stdlib.h>
#include <string.h>

extern int encode(char *filename, opj_image_t *image);
extern opj_image_t *decode(char *filename);

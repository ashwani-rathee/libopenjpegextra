#include"libopenjpegextra.h"
#define J2K_CFMT 0
#define JP2_CFMT 1
#define JPT_CFMT 2

#define PXM_DFMT 10
#define PGX_DFMT 11
#define BMP_DFMT 12
#define YUV_DFMT 13
#define TIF_DFMT 14
#define RAW_DFMT 15 /* MSB / Big Endian */
#define TGA_DFMT 16
#define PNG_DFMT 17
#define RAWL_DFMT 18 /* LSB / Little Endian */
typedef int opj_bool;


typedef enum opj_prec_mode {
    OPJ_PREC_MODE_CLIP,
    OPJ_PREC_MODE_SCALE
} opj_precision_mode;

typedef struct opj_prec {
    OPJ_UINT32         prec;
    opj_precision_mode mode;
} opj_precision;


typedef struct opj_decompress_params {
    /** core library parameters */
    opj_dparameters_t core;

    /** input file name */
    char infile[OPJ_PATH_LEN];
    /** output file name */
    char outfile[OPJ_PATH_LEN];
    /** input file format 0: J2K, 1: JP2, 2: JPT */
    int decod_format;
    /** output file format 0: PGX, 1: PxM, 2: BMP */
    int cod_format;
    /** index file name */
    char indexfilename[OPJ_PATH_LEN];

    /** Decoding area left boundary */
    OPJ_UINT32 DA_x0;
    /** Decoding area right boundary */
    OPJ_UINT32 DA_x1;
    /** Decoding area up boundary */
    OPJ_UINT32 DA_y0;
    /** Decoding area bottom boundary */
    OPJ_UINT32 DA_y1;
    /** Verbose mode */
    OPJ_BOOL m_verbose;

    /** tile number of the decoded tile */
    OPJ_UINT32 tile_index;
    /** Nb of tile to decode */
    OPJ_UINT32 nb_tile_to_decode;

    opj_precision* precision;
    OPJ_UINT32     nb_precision;

    /* force output colorspace to RGB */
    int force_rgb;
    /* upsample components according to their dx/dy values */
    int upsample;
    /* split output components to different files */
    int split_pnm;
    /** number of threads */
    int num_threads;
    /* Quiet */
    int quiet;
    /* Allow partial decode */
    int allow_partial;
    /** number of components to decode */
    OPJ_UINT32 numcomps;
    /** indices of components to decode */
    OPJ_UINT32* comps_indices;
} opj_decompress_parameters;

/**
Divide an integer by a power of 2 and round upwards
@return Returns a divided by 2^b
*/
static int int_ceildivpow2(int a, int b)
{
    return (a + (1 << b) - 1) >> b;
}

#define READ_JP2 1
#define WRITE_JP2 0

typedef struct raw_cparameters {
    /** width of the raw image */
    int rawWidth;
    /** height of the raw image */
    int rawHeight;
    /** components of the raw image */
    int rawComp;
    /** bit depth of the raw image */
    int rawBitDepth;
    /** signed/unsigned raw image */
    opj_bool rawSigned;
    /*@}*/
} raw_cparameters_t;

//


static void set_default_parameters(opj_decompress_parameters* parameters)
{
    if (parameters) {
        memset(parameters, 0, sizeof(opj_decompress_parameters));

        /* default decoding parameters (command line specific) */
        parameters->decod_format = -1;
        parameters->cod_format = -1;

        /* default decoding parameters (core) */
        opj_set_default_decoder_parameters(&(parameters->core));
    }
}

static void destroy_parameters(opj_decompress_parameters* parameters)
{
    if (parameters) {
        if (parameters->precision) {
            free(parameters->precision);
            parameters->precision = NULL;
        }

        free(parameters->comps_indices);
        parameters->comps_indices = NULL;
    }
}

int get_file_format(const char *filename)
{
    unsigned int i;
    static const char * const extension[] = {
        "pgx", "pnm", "pgm", "ppm", "bmp",
        "tif", "tiff",
        "raw", "yuv", "rawl",
        "tga", "png",
        "j2k", "jp2", "jpt", "j2c", "jpc",
        "jph", /* HTJ2K with JP2 boxes */
        "jhc" /* HTJ2K codestream */
    };
    static const int format[] = {
        PGX_DFMT, PXM_DFMT, PXM_DFMT, PXM_DFMT, BMP_DFMT,
        TIF_DFMT, TIF_DFMT,
        RAW_DFMT, RAW_DFMT, RAWL_DFMT,
        TGA_DFMT, PNG_DFMT,
        J2K_CFMT, JP2_CFMT, JPT_CFMT, J2K_CFMT, J2K_CFMT,
        JP2_CFMT, /* HTJ2K with JP2 boxes */
        J2K_CFMT /* HTJ2K codestream */
    };
    const char * ext = strrchr(filename, '.');
    if (ext == NULL) {
        return -1;
    }
    ext++;
    if (*ext) {
        for (i = 0; i < sizeof(format) / sizeof(*format); i++) {
            if (strcasecmp(ext, extension[i]) == 0) {
                return format[i];
            }
        }
    }

    return -1;
}

/* -------------------------------------------------------------------------- */
#define JP2_RFC3745_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC "\x0d\x0a\x87\x0a"
/* position 45: "\xff\x52" */
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"

static int infile_format(const char *fname)
{
    FILE *reader;
    const char *s, *magic_s;
    int ext_format, magic_format;
    unsigned char buf[12];
    OPJ_SIZE_T l_nb_read;

    reader = fopen(fname, "rb");

    if (reader == NULL) {
        return -2;
    }

    memset(buf, 0, 12);
    l_nb_read = fread(buf, 1, 12, reader);
    fclose(reader);
    if (l_nb_read != 12) {
        return -1;
    }



    ext_format = get_file_format(fname);

    if (ext_format == JPT_CFMT) {
        return JPT_CFMT;
    }

    if (memcmp(buf, JP2_RFC3745_MAGIC, 12) == 0 || memcmp(buf, JP2_MAGIC, 4) == 0) {
        magic_format = JP2_CFMT;
        magic_s = ".jp2 or .jph";
    } else if (memcmp(buf, J2K_CODESTREAM_MAGIC, 4) == 0) {
        magic_format = J2K_CFMT;
        magic_s = ".j2k or .jpc or .j2c or .jhc";
    } else {
        return -1;
    }

    if (magic_format == ext_format) {
        return ext_format;
    }

    s = fname + strlen(fname) - 4;

    fputs("\n===========================================\n", stderr);
    fprintf(stderr, "The extension of this file is incorrect.\n"
            "FOUND %s. SHOULD BE %s\n", s, magic_s);
    fputs("===========================================\n", stderr);

    return magic_format;
}

//
int encode(char *filename, opj_image_t *image)
{
    opj_cparameters_t parameters;   /* compression parameters */

    opj_stream_t *l_stream = 00;
    opj_codec_t* l_codec = 00;
    // opj_image_t *image = NULL;
    raw_cparameters_t raw_cp;
    OPJ_UINT32 l_nb_tiles = 4;
    int framerate = 0;

    OPJ_BOOL bSuccess;
    OPJ_BOOL bUseTiles = OPJ_FALSE; /* OPJ_TRUE */
    int num_threads = 0;

    /* set encoding parameters to default values */
    opj_set_default_encoder_parameters(&parameters);
    /* raw_cp initialization */
    raw_cp.rawBitDepth = 0;
    raw_cp.rawComp = 0;
    raw_cp.rawHeight = 0;
    raw_cp.rawSigned = 0;
    raw_cp.rawWidth = 0;
    int ret = 0;
    parameters.tcp_mct = (char)255; /* This will be set later according to the input image or the provided option */
    if (!image) {
            fprintf(stderr, "Unable to load file: got no image\n");
            ret = 1;
            goto fin;
    }

    /* Decide if MCT should be used */
    if (parameters.tcp_mct == (char)255) 
    { /* mct mode has not been set in commandline */
        parameters.tcp_mct = (image->numcomps >= 3) ? 1 : 0;
    } 
    else 
    {
        /* mct mode has been set in commandline */
        if ((parameters.tcp_mct == 1) && (image->numcomps < 3)) {
            fprintf(stderr, "RGB->YCC conversion cannot be used:\n");
            fprintf(stderr, "Input image has less than 3 components\n");
            ret = 1;
            goto fin;
        }
        if ((parameters.tcp_mct == 2) && (!parameters.mct_data)) {
            fprintf(stderr, "Custom MCT has been set but no array-based MCT\n");
            fprintf(stderr, "has been provided. Aborting.\n");
            ret = 1;
            goto fin;
        }
    }

    if (OPJ_IS_IMF(parameters.rsiz) && framerate > 0) {
    const int mainlevel = OPJ_GET_IMF_MAINLEVEL(parameters.rsiz);
    if (mainlevel > 0 && mainlevel <= OPJ_IMF_MAINLEVEL_MAX) {
        const int limitMSamplesSec[] = {
            0,
            OPJ_IMF_MAINLEVEL_1_MSAMPLESEC,
            OPJ_IMF_MAINLEVEL_2_MSAMPLESEC,
            OPJ_IMF_MAINLEVEL_3_MSAMPLESEC,
            OPJ_IMF_MAINLEVEL_4_MSAMPLESEC,
            OPJ_IMF_MAINLEVEL_5_MSAMPLESEC,
            OPJ_IMF_MAINLEVEL_6_MSAMPLESEC,
            OPJ_IMF_MAINLEVEL_7_MSAMPLESEC,
            OPJ_IMF_MAINLEVEL_8_MSAMPLESEC,
            OPJ_IMF_MAINLEVEL_9_MSAMPLESEC,
            OPJ_IMF_MAINLEVEL_10_MSAMPLESEC,
            OPJ_IMF_MAINLEVEL_11_MSAMPLESEC
        };
        OPJ_UINT32 avgcomponents = image->numcomps;
        double msamplespersec;
        if (image->numcomps == 3 &&
                image->comps[1].dx == 2 &&
                image->comps[1].dy == 2) {
            avgcomponents = 2;
        }
        msamplespersec = (double)image->x1 * image->y1 * avgcomponents * framerate /
                            1e6;
        if (msamplespersec > limitMSamplesSec[mainlevel]) {
            fprintf(stderr,
                    "Warning: MSamples/sec is %f, whereas limit is %d.\n",
                    msamplespersec,
                    limitMSamplesSec[mainlevel]);
        }
    }
    }

    /* encode the destination image */
    /* ---------------------------- */
    parameters.cod_format = get_file_format(filename);
    switch (parameters.cod_format) {
        case J2K_CFMT: { /* JPEG-2000 codestream */
            /* Get a decoder handle */
            l_codec = opj_create_compress(OPJ_CODEC_J2K);
            break;
        }
        case JP2_CFMT: { /* JPEG 2000 compressed image data */
            /* Get a decoder handle */
            l_codec = opj_create_compress(OPJ_CODEC_JP2);
            break;
        }
        default:
            fprintf(stderr, "skipping file..\n");
            opj_stream_destroy(l_stream);
            return 1;
        }
    if (bUseTiles) {
            parameters.cp_tx0 = 0;
            parameters.cp_ty0 = 0;
            parameters.tile_size_on = OPJ_TRUE;
            parameters.cp_tdx = 512;
            parameters.cp_tdy = 512;
    }
    
    if (! opj_setup_encoder(l_codec, &parameters, image)) {
            fprintf(stderr, "failed to encode image: opj_setup_encoder\n");
            opj_destroy_codec(l_codec);
            opj_image_destroy(image);
            ret = 1;
            goto fin;
    }
    OPJ_BOOL PLT = OPJ_FALSE;
    if (PLT) {
        const char* const options[] = { "PLT=YES", NULL };
        if (!opj_encoder_set_extra_options(l_codec, options)) {
            fprintf(stderr, "failed to encode image: opj_encoder_set_extra_options\n");
            opj_destroy_codec(l_codec);
            opj_image_destroy(image);
            ret = 1;
            goto fin;
        }
    }

    if (num_threads >= 1 &&
            !opj_codec_set_threads(l_codec, num_threads)) {
        fprintf(stderr, "failed to set number of threads\n");
        opj_destroy_codec(l_codec);
        opj_image_destroy(image);
        ret = 1;
        goto fin;
    }

    /* open a byte stream for writing and allocate memory for all tiles */
    l_stream = opj_stream_create_default_file_stream(filename, OPJ_FALSE);
    if (! l_stream) {
        ret = 1;
        goto fin;
    }

    /* encode the image */
    bSuccess = opj_start_compress(l_codec, image, l_stream);
    if (!bSuccess)  {
        fprintf(stderr, "failed to encode image: opj_start_compress\n");
    }
    if (bSuccess && bUseTiles) {
        OPJ_BYTE *l_data;
        OPJ_UINT32 l_data_size = 512 * 512 * 3;
        l_data = (OPJ_BYTE*) calloc(1, l_data_size);
        if (l_data == NULL) {
            ret = 1;
            goto fin;
        }
        for (int i = 0; i < l_nb_tiles; ++i) {
            if (! opj_write_tile(l_codec, i, l_data, l_data_size, l_stream)) {
                fprintf(stderr, "ERROR -> test_tile_encoder: failed to write the tile %d!\n",
                        i);
                opj_stream_destroy(l_stream);
                opj_destroy_codec(l_codec);
                opj_image_destroy(image);
                ret = 1;
                goto fin;
            }
        }
        free(l_data);
    } else {
        bSuccess = bSuccess && opj_encode(l_codec, l_stream);
        if (!bSuccess)  {
            fprintf(stderr, "failed to encode image: opj_encode\n");
        }
    }
    bSuccess = bSuccess && opj_end_compress(l_codec, l_stream);
    if (!bSuccess)  {
        fprintf(stderr, "failed to encode image: opj_end_compress\n");
    }

    if (!bSuccess)  {
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        opj_image_destroy(image);
        fprintf(stderr, "failed to encode image\n");
        remove(parameters.outfile);
        ret = 1;
        goto fin;
    }

    /* close and free the byte stream */
    opj_stream_destroy(l_stream);

    /* free remaining compression structures */
    opj_destroy_codec(l_codec);

    /* free image data */
    opj_image_destroy(image);
    ret = 0;

fin:
    if (parameters.cp_comment) {
        free(parameters.cp_comment);
    }
    if (parameters.cp_matrice) {
        free(parameters.cp_matrice);
    }
    if (raw_cp.rawComp) {
        free(raw_cp.rawComp);
    }
    return -1;
}


opj_image_t *decode(char *filename)
{
    opj_decompress_parameters parameters;           /* decompression parameters */
    set_default_parameters(&parameters);
    parameters.decod_format = infile_format(filename);
    int failed = 0;
    opj_image_t *image = NULL;
    opj_stream_t *l_stream = NULL; /* Stream */
    opj_codec_t *l_codec = NULL;   /* Handle to a decompressor */
    opj_codestream_index_t *cstr_index = NULL;

    l_stream = opj_stream_create_default_file_stream(filename, 1);
    if (!l_stream) {
        fprintf(stderr, "ERROR -> failed to create the stream from the file %s\n",
                parameters.infile);
        destroy_parameters(&parameters);
        return NULL;
    }

    switch (parameters.decod_format) {
        case J2K_CFMT: { /* JPEG-2000 codestream */
            /* Get a decoder handle */
            l_codec = opj_create_decompress(OPJ_CODEC_J2K);
            break;
        }
        case JP2_CFMT: { /* JPEG 2000 compressed image data */
            /* Get a decoder handle */
            l_codec = opj_create_decompress(OPJ_CODEC_JP2);
            break;
        }
        case JPT_CFMT: { /* JPEG 2000, JPIP */
            /* Get a decoder handle */
            l_codec = opj_create_decompress(OPJ_CODEC_JPT);
            break;
    }
    default:
        fprintf(stderr, "Unexpected format!\n");
        destroy_parameters(&parameters);
        opj_stream_destroy(l_stream);
        return NULL;
    }

    if (! opj_read_header(l_stream, l_codec, &image)) {
        fprintf(stderr, "ERROR -> opj_decompress: failed to read the header\n");
        opj_stream_destroy(l_stream);
        opj_destroy_codec(l_codec);
        opj_image_destroy(image);
        destroy_parameters(&parameters);
        return NULL;
    }

    if (parameters.numcomps) {
            if (! opj_set_decoded_components(l_codec,
                                             parameters.numcomps,
                                             parameters.comps_indices,
                                             OPJ_FALSE)) {
                fprintf(stderr,
                        "ERROR -> opj_decompress: failed to set the component indices!\n");
                opj_destroy_codec(l_codec);
                opj_stream_destroy(l_stream);
                opj_image_destroy(image);
                destroy_parameters(&parameters);
                return NULL;
            }
        }
    if (!(opj_decode(l_codec, l_stream, image) && opj_end_decompress(l_codec,   l_stream))) {
        fprintf(stderr, "ERROR -> opj_decompress: failed to decode image!\n");
        opj_destroy_codec(l_codec);
        opj_stream_destroy(l_stream);
        opj_image_destroy(image);
        destroy_parameters(&parameters);
        return NULL;
    }

    if (image->color_space != OPJ_CLRSPC_SYCC
                && image->numcomps == 3 && image->comps[0].dx == image->comps[0].dy
                && image->comps[1].dx != 1) {
            image->color_space = OPJ_CLRSPC_SYCC;
    } 
    else if (image->numcomps <= 2) {
            image->color_space = OPJ_CLRSPC_GRAY;
    }

            /* free remaining structures */
    if (l_codec) {
            opj_destroy_codec(l_codec);
    }
    opj_stream_destroy(l_stream);
    return image;
}

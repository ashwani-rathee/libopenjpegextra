#include<libopenjpegextra.h>
#define J2K_CFMT 0
#define JP2_CFMT 1
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

opj_image_t *convert_gray_to_rgb(opj_image_t *original)
{
    OPJ_UINT32 compno;
    opj_image_t *l_new_image = NULL;
    opj_image_cmptparm_t *l_new_components = NULL;

    l_new_components = (opj_image_cmptparm_t *)malloc((original->numcomps + 2U) *
                                                      sizeof(opj_image_cmptparm_t));
    if (l_new_components == NULL)
    {
        fprintf(stderr,
                "ERROR -> opj_decompress: failed to allocate memory for RGB image!\n");
        opj_image_destroy(original);
        return NULL;
    }

    l_new_components[0].dx = l_new_components[1].dx = l_new_components[2].dx =
        original->comps[0].dx;
    l_new_components[0].dy = l_new_components[1].dy = l_new_components[2].dy =
        original->comps[0].dy;
    l_new_components[0].h = l_new_components[1].h = l_new_components[2].h =
        original->comps[0].h;
    l_new_components[0].w = l_new_components[1].w = l_new_components[2].w =
        original->comps[0].w;
    l_new_components[0].prec = l_new_components[1].prec = l_new_components[2].prec =
        original->comps[0].prec;
    l_new_components[0].sgnd = l_new_components[1].sgnd = l_new_components[2].sgnd =
        original->comps[0].sgnd;
    l_new_components[0].x0 = l_new_components[1].x0 = l_new_components[2].x0 =
        original->comps[0].x0;
    l_new_components[0].y0 = l_new_components[1].y0 = l_new_components[2].y0 =
        original->comps[0].y0;

    for (compno = 1U; compno < original->numcomps; ++compno)
    {
        l_new_components[compno + 2U].dx = original->comps[compno].dx;
        l_new_components[compno + 2U].dy = original->comps[compno].dy;
        l_new_components[compno + 2U].h = original->comps[compno].h;
        l_new_components[compno + 2U].w = original->comps[compno].w;
        l_new_components[compno + 2U].prec = original->comps[compno].prec;
        l_new_components[compno + 2U].sgnd = original->comps[compno].sgnd;
        l_new_components[compno + 2U].x0 = original->comps[compno].x0;
        l_new_components[compno + 2U].y0 = original->comps[compno].y0;
    }

    l_new_image = opj_image_create(original->numcomps + 2U, l_new_components,
                                   OPJ_CLRSPC_SRGB);
    free(l_new_components);
    if (l_new_image == NULL)
    {
        fprintf(stderr,
                "ERROR -> opj_decompress: failed to allocate memory for RGB image!\n");
        opj_image_destroy(original);
        return NULL;
    }

    l_new_image->x0 = original->x0;
    l_new_image->x1 = original->x1;
    l_new_image->y0 = original->y0;
    l_new_image->y1 = original->y1;

    l_new_image->comps[0].factor = l_new_image->comps[1].factor =
        l_new_image->comps[2].factor = original->comps[0].factor;
    l_new_image->comps[0].alpha = l_new_image->comps[1].alpha =
        l_new_image->comps[2].alpha = original->comps[0].alpha;
    l_new_image->comps[0].resno_decoded = l_new_image->comps[1].resno_decoded =
        l_new_image->comps[2].resno_decoded = original->comps[0].resno_decoded;

    memcpy(l_new_image->comps[0].data, original->comps[0].data,
           sizeof(OPJ_INT32) * original->comps[0].w * original->comps[0].h);
    memcpy(l_new_image->comps[1].data, original->comps[0].data,
           sizeof(OPJ_INT32) * original->comps[0].w * original->comps[0].h);
    memcpy(l_new_image->comps[2].data, original->comps[0].data,
           sizeof(OPJ_INT32) * original->comps[0].w * original->comps[0].h);

    for (compno = 1U; compno < original->numcomps; ++compno)
    {
        l_new_image->comps[compno + 2U].factor = original->comps[compno].factor;
        l_new_image->comps[compno + 2U].alpha = original->comps[compno].alpha;
        l_new_image->comps[compno + 2U].resno_decoded =
            original->comps[compno].resno_decoded;
        memcpy(l_new_image->comps[compno + 2U].data, original->comps[compno].data,
               sizeof(OPJ_INT32) * original->comps[compno].w * original->comps[compno].h);
    }
    opj_image_destroy(original);
    return l_new_image;
}

//
// RGB buffer to  jp2 file
//
int encode(char *filename, opj_image_t *image)
{
    printf("Encode\n");
    // printf("%d %d %d\n", image->comps[0].dx, image->comps[0].dy, image->comps[0].prec);
    opj_cparameters_t parameters;
    // opj_image_t *image;
    int sub_dx, sub_dy;
    OPJ_CODEC_FORMAT codec_format;
    codec_format = OPJ_CODEC_JP2;
    opj_set_default_encoder_parameters(&parameters);

    if (parameters.cp_comment == NULL)
    {
        char buf[80];
#ifdef _WIN32
        sprintf_s(buf, 80, "Created by OpenJPEG version %s", opj_version());
#else
        snprintf(buf, 80, "Created by OpenJPEG version %s", opj_version());
#endif
        parameters.cp_comment = strdup(buf);
    }

    if (parameters.tcp_numlayers == 0)
    {
        parameters.tcp_rates[0] = 0; /* MOD antonin : losslessbug */
        parameters.tcp_numlayers++;
        parameters.cp_disto_alloc = 1;
    }
    sub_dx = parameters.subsampling_dx;
    sub_dy = parameters.subsampling_dy;
    codec_format = OPJ_CODEC_JP2;
    if (image == NULL)
    {
        fprintf(stderr, "%d: write_jp2_file fails.\n", __LINE__);

        if (parameters.cp_comment)
            free(parameters.cp_comment);
        return 0;
    }
    {
        opj_stream_t *stream;
        opj_codec_t *codec;
        stream = NULL;
        parameters.tcp_mct = image->numcomps == 3 ? 1 : 0;
        codec = opj_create_compress(codec_format);
        if (codec == NULL)
            goto fin;
        opj_setup_encoder(codec, &parameters, image);
        stream =
            opj_stream_create_default_file_stream(filename, WRITE_JP2);
        if (stream == NULL)
            goto fin;
        if (!opj_start_compress(codec, image, stream))
            goto fin;
        if (!opj_encode(codec, stream))
            goto fin;
        opj_end_compress(codec, stream);

    fin:
        if (stream)
            opj_stream_destroy(stream);
        if (codec)
            opj_destroy_codec(codec);
        opj_image_destroy(image);
        if (parameters.cp_comment)
            free(parameters.cp_comment);
    }
}

//
// jp2 file to RGB buffer
//
opj_image_t *decode(char *filename)
{
    opj_image_t *image = NULL;
    opj_stream_t *l_stream = NULL; /* Stream */
    opj_codec_t *l_codec = NULL;   /* Handle to a decompressor */
    opj_codestream_index_t *cstr_index = NULL;

    l_codec = opj_create_decompress(OPJ_CODEC_JP2);
    l_stream = opj_stream_create_default_file_stream(filename, 1);
    opj_read_header(l_stream, l_codec, &image);
    opj_decode(l_codec, l_stream, image);
    printf("Decoding %s\n", filename);
    // opj_stream_destroy(l_stream);
    // opj_destroy_codec(l_codec);
    // opj_image_destroy(image);
    return image;
}



rgb imagetorgbbuffer(opj_image_t *image)
{
    rgb data;
    data.r = image->comps[0].data;
    data.g = image->comps[1].data;
    data.b = image->comps[2].data;
    // for(int i=0; i<2717*3701; i++)
    // {
    //     printf("%d \n ", r[i]);
    // }
    return data;
}

opj_image_t *rgbbuffertoimage(opj_image_t *original, rgb buf)
{
    printf("RGB buffer to OPJ_IMAGE_T test\n");
    OPJ_UINT32 compno;
    opj_image_t *image = NULL;
    opj_image_cmptparm_t *l_new_components = NULL;

    l_new_components = (opj_image_cmptparm_t *)malloc((original->numcomps + 2U) *
                                                      sizeof(opj_image_cmptparm_t));
    if (l_new_components == NULL)
    {
        fprintf(stderr,
                "ERROR -> opj_decompress: failed to allocate memory for RGB image!\n");
        opj_image_destroy(original);
        return NULL;
    }

    l_new_components[0].dx = l_new_components[1].dx = l_new_components[2].dx =
        original->comps[0].dx;
    l_new_components[0].dy = l_new_components[1].dy = l_new_components[2].dy =
        original->comps[0].dy;
    l_new_components[0].h = l_new_components[1].h = l_new_components[2].h =
        original->comps[0].h;
    l_new_components[0].w = l_new_components[1].w = l_new_components[2].w =
        original->comps[0].w;
    l_new_components[0].prec = l_new_components[1].prec = l_new_components[2].prec =
        original->comps[0].prec;
    l_new_components[0].sgnd = l_new_components[1].sgnd = l_new_components[2].sgnd =
        original->comps[0].sgnd;
    l_new_components[0].x0 = l_new_components[1].x0 = l_new_components[2].x0 =
        original->comps[0].x0;
    l_new_components[0].y0 = l_new_components[1].y0 = l_new_components[2].y0 =
        original->comps[0].y0;

    for (compno = 1U; compno < original->numcomps; ++compno)
    {
        l_new_components[compno + 2U].dx = original->comps[compno].dx;
        l_new_components[compno + 2U].dy = original->comps[compno].dy;
        l_new_components[compno + 2U].h = original->comps[compno].h;
        l_new_components[compno + 2U].w = original->comps[compno].w;
        l_new_components[compno + 2U].prec = original->comps[compno].prec;
        l_new_components[compno + 2U].sgnd = original->comps[compno].sgnd;
        l_new_components[compno + 2U].x0 = original->comps[compno].x0;
        l_new_components[compno + 2U].y0 = original->comps[compno].y0;
    }

    image = opj_image_create(original->numcomps + 2U, l_new_components,
                                   OPJ_CLRSPC_SRGB);

    // image = opj_image_create(3U, &cmptparm, color_space);
    if (!image)
    {
        fprintf(stderr, "ERROR -> failed to create the image\n");
        return NULL;
    }

    printf("Time to fill in data: \n");
    for (int i = 0; i < original->comps[0].w * original->comps[0].h; i++)
    {
        unsigned int compno;
        for (compno = 0; compno < original->numcomps + 2U; compno++)
        {
            if (compno == 0)
            {
                image->comps[compno].data[i] = buf.r[i];
            }
            else if (compno == 1)
            {
                image->comps[compno].data[i] = buf.g[i];
            }
            else if (compno == 2)
            {
                image->comps[compno].data[i] = buf.b[i];
            }
        }
    }
    printf("Data fill Done: \n");
    return image;
}

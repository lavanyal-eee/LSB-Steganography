#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "decode.h"
#include "types.h"
#include "common.h"

/* Helper: read N bytes into buffer, return number actually read */
static size_t safe_fread(void *ptr, size_t nmemb, FILE *f)
{
    return fread(ptr, 1, nmemb, f);
}

/* Helper: decode one byte encoded in 8 image bytes (MSB-first) */
static Status decode_byte_from_image(FILE *fptr_src, unsigned char *out)
{
    unsigned char buf[8];
    if (safe_fread(buf, 8, fptr_src) != 8)
        return e_failure;

    unsigned char ch = 0;
    for (int i = 0; i < 8; ++i)
    {
        ch = (ch << 1) | (buf[i] & 0x01);
    }
    *out = ch;
    return e_success;
}

/* Helper: decode 32-bit unsigned int encoded in 32 image bytes (MSB-first) */
static Status decode_uint32_from_image(FILE *fptr_src, uint *out)
{
    unsigned char buf[32];
    if (safe_fread(buf, 32, fptr_src) != 32)
        return e_failure;

    unsigned int value = 0;
    for (int i = 0; i < 32; ++i)
    {
        value = (value << 1) | (buf[i] & 0x01);
    }
    *out = value;
    return e_success;
}

/* Validate args: argv[2] = stego.bmp, optional argv[3] = output file */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    if (argv == NULL || argv[2] == NULL)
    {
        fprintf(stderr, "ERROR: Missing stego image argument\n");
        return e_failure;
    }

    char *dot = strstr(argv[2], ".");
    if (dot == NULL || strcmp(dot, ".bmp") != 0)
    {
        fprintf(stderr, "ERROR: Stego image must be a .bmp file\n");
        return e_failure;
    }

    decInfo->stego_image_fname = argv[2];

    if (argv[3] != NULL)
        decInfo->output_fname = argv[3];
    else
        decInfo->output_fname = "decoded.txt";

    /* init */
    memset(decInfo->magic_string, 0, sizeof(decInfo->magic_string));
    memset(decInfo->extn_secret_file, 0, sizeof(decInfo->extn_secret_file));
    decInfo->size_secret_file = 0;

    return e_success;
}

/* Open files */
Status open_decode_files(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "r");
    if (decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open %s\n", decInfo->stego_image_fname);
        return e_failure;
    }

    /* We will create the output file only after we know the extension/name.
       However to match the teacher's flow we can open the provided output_fname now. */
    decInfo->fptr_output = fopen(decInfo->output_fname, "w");
    if (decInfo->fptr_output == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open %s\n", decInfo->output_fname);
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }

    return e_success;
}

/* Decode magic string: seek to BMP data (54 bytes), read len(MAGIC_STRING) bytes */
Status decode_magic_string(DecodeInfo *decInfo)
{
    /* move to pixel data start: encoder copied header (54 bytes) then encoded */
    if (fseek(decInfo->fptr_stego_image, 54, SEEK_SET) != 0)
        return e_failure;

    for (size_t i = 0; i < MAGIC_STR_SIZE; ++i)
    {
        unsigned char ch;
        if (decode_byte_from_image(decInfo->fptr_stego_image, &ch) != e_success)
            return e_failure;
        decInfo->magic_string[i] = (char)ch;
    }
    decInfo->magic_string[MAGIC_STR_SIZE] = '\0';

    if (strcmp(decInfo->magic_string, MAGIC_STRING) != 0)
    {
        fprintf(stderr, "ERROR: Magic string mismatch (found '%s')\n", decInfo->magic_string);
        return e_failure;
    }

    return e_success;
}

/* Decode the secret file extension size (32 bits) */
Status decode_secret_file_extn_size(DecodeInfo *decInfo)
{
    uint extn_size = 0;
    if (decode_uint32_from_image(decInfo->fptr_stego_image, &extn_size) != e_success)
        return e_failure;

    if (extn_size >= MAX_FILE_EXTN)
    {
        fprintf(stderr, "ERROR: decoded extension size too large: %u\n", extn_size);
        return e_failure;
    }

    /* store in extn buffer length temporarily in size_secret_file (we'll reuse fields later) */
    /* We'll store the extn size inside size_secret_file temporarily */
    decInfo->size_secret_file = extn_size; /* reused temporarily */
    return e_success;
}

/* Decode the secret file extension string of decoded length */
Status decode_secret_file_extn(DecodeInfo *decInfo)
{
    uint extn_size = decInfo->size_secret_file; /* temp */
    for (uint i = 0; i < extn_size; ++i)
    {
        unsigned char ch;
        if (decode_byte_from_image(decInfo->fptr_stego_image, &ch) != e_success)
            return e_failure;
        decInfo->extn_secret_file[i] = (char)ch;
    }
    decInfo->extn_secret_file[extn_size] = '\0';
    return e_success;
}

/* Decode secret file size (32 bits) */
Status decode_secret_file_size(DecodeInfo *decInfo)
{
    uint secret_size = 0;
    if (decode_uint32_from_image(decInfo->fptr_stego_image, &secret_size) != e_success)
        return e_failure;

    decInfo->size_secret_file = secret_size;
    return e_success;
}

/* Decode secret file data and write to output file */
Status decode_secret_file_data(DecodeInfo *decInfo)
{
    uint n = decInfo->size_secret_file;
    if (n == 0)
    {
        /* nothing to decode */
        return e_success;
    }

    /* If the user provided a generic output name earlier and we want
       to change filename to include extension decoded, we could reopen.
       For simplicity: we already opened output file using output_fname.
       If you prefer creating filename using decoded extension, uncomment logic below. */

    /* Write all decoded bytes */
    for (uint i = 0; i < n; ++i)
    {
        unsigned char ch;
        if (decode_byte_from_image(decInfo->fptr_stego_image, &ch) != e_success)
            return e_failure;

        if (fwrite(&ch, 1, 1, decInfo->fptr_output) != 1)
            return e_failure;
    }

    return e_success;
}

/* Top-level decoding flow */
Status do_decoding(DecodeInfo *decInfo)
{
    if (open_decode_files(decInfo) != e_success)
    {
        fprintf(stderr, "ERROR: opening files failed\n");
        return e_failure;
    }

    /* 1: decode magic string */
    if (decode_magic_string(decInfo) != e_success)
    {
        fprintf(stderr, "ERROR: decode magic string failed\n");
        fclose(decInfo->fptr_stego_image);
        fclose(decInfo->fptr_output);
        return e_failure;
    }

    /* 2: decode extension size (32 bits) */
    if (decode_secret_file_extn_size(decInfo) != e_success)
    {
        fprintf(stderr, "ERROR: decode extn size failed\n");
        fclose(decInfo->fptr_stego_image);
        fclose(decInfo->fptr_output);
        return e_failure;
    }

    /* 3: decode extension string */
    if (decode_secret_file_extn(decInfo) != e_success)
    {
        fprintf(stderr, "ERROR: decode extn failed\n");
        fclose(decInfo->fptr_stego_image);
        fclose(decInfo->fptr_output);
        return e_failure;
    }

    /* 4: decode secret file size (32 bits) */
    if (decode_secret_file_size(decInfo) != e_success)
    {
        fprintf(stderr, "ERROR: decode secret file size failed\n");
        fclose(decInfo->fptr_stego_image);
        fclose(decInfo->fptr_output);
        return e_failure;
    }

    /* 5: decode secret data and write to output */
    if (decode_secret_file_data(decInfo) != e_success)
    {
        fprintf(stderr, "ERROR: decode secret file data failed\n");
        fclose(decInfo->fptr_stego_image);
        fclose(decInfo->fptr_output);
        return e_failure;
    }

    /* close files */
    fclose(decInfo->fptr_stego_image);
    fclose(decInfo->fptr_output);

    return e_success;
}

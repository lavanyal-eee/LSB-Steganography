#ifndef DECODE_H
#define DECODE_H

#include "types.h"   /* uses Status, uint */
#include "common.h"  /* uses MAGIC_STRING, maybe other constants */

#define MAGIC_STR_SIZE (sizeof(MAGIC_STRING) - 1)
#define MAX_FILE_EXTN 16

typedef struct _DecodeInfo
{
    char *stego_image_fname;
    FILE *fptr_stego_image;

    char *output_fname;
    FILE *fptr_output;

    char magic_string[MAGIC_STR_SIZE + 1];
    char extn_secret_file[MAX_FILE_EXTN];
    uint size_secret_file;
} DecodeInfo;


Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo);
Status open_decode_files(DecodeInfo *decInfo);
Status do_decoding(DecodeInfo *decInfo);

Status decode_magic_string(DecodeInfo *decInfo);
Status decode_secret_file_extn_size(DecodeInfo *decInfo);
Status decode_secret_file_extn(DecodeInfo *decInfo);
Status decode_secret_file_size(DecodeInfo *decInfo);
Status decode_secret_file_data(DecodeInfo *decInfo);

#endif /* DECODE_H */

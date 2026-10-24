#include <stdio.h>
#include <stdlib.h>
#include "decode.h"
#include "types.h"
#include "common.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s -d <stego_image.bmp> [output_file]\n", argv[0]);
        return 1;
    }

    DecodeInfo decInfo;
    Status status;

    printf("Starting Decoding Process...\n");

    /* Step 1: Read and validate arguments */
    status = read_and_validate_decode_args(argv, &decInfo);
    if (status != e_success)
    {
        fprintf(stderr, "Error: Invalid arguments for decoding.\n");
        return 1;
    }

    /* Step 2: Perform decoding */
    status = do_decoding(&decInfo);
    if (status == e_success)
    {
        printf("Decoding Successful!\n");
        printf("Magic String: %s\n", decInfo.magic_string);
        printf("File Extension: %s\n", decInfo.extn_secret_file);
        printf("Secret File Size: %u bytes\n", decInfo.size_secret_file);
        printf("Decoded output written to: %s\n", decInfo.output_fname);
    }
    else
    {
        fprintf(stderr, "Decoding Failed.\n");
        return 1;
    }

    return 0;
}

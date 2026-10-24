//TEST_ENCODE.C FILE

#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "types.h"

int main(int argc, char *argv[])
{
    if (check_operation_type(argv) == e_encode)
  {
    EncodeInfo encInfo;
    printf("Selected Encoding\n");

    Status status = read_and_validate_encode_args(argv, &encInfo);
    printf("Status from read_and_validate_encode_args = %d\n", status);

    if (status == e_success)
    {
        printf("Read and validate encode arguments is success\n");
        if (do_encoding(&encInfo) == e_success)
        {
            printf("Completed Encoding\n");
        }
        else
        {
            printf("Failed to do the encoding\n");
        }
    }
    else
    {
        printf("Read and validate encode arguments failed\n");
    }
    }

    else if (check_operation_type(argv) == e_decode)
    {
        printf("Selected Decoding\n");
    }
    else
    {
        printf("Invalid Arguments\n");
        printf("******************* USAGE ******************\n");
        printf("Encoding: ./a.out -e beautiful.bmp secret.txt stego.bmp\n");
        printf("Decoding: ./a.out -d stego.bmp decode.txt\n");
    }
    return 0;
}
OperationType check_operation_type(char *argv[])
{
    if(strcmp(argv[1],"-e") == 0)
        return e_encode;
    else if(strcmp(argv[1],"-d") == 0)
        return e_decode;
    else
        return e_unsupported;

}
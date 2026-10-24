//ENCODE.C FILE
#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "types.h"
#include "common.h"

/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    // Return image capacity
    return width * height * 3;
}

/* 
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files(EncodeInfo *encInfo)
{
    // Src Image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    // Do Error handling
    if (encInfo->fptr_src_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);

    	return e_failure;
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    // Do Error handling
    if (encInfo->fptr_secret == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);

    	return e_failure;
    }

    // Stego Image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    // Do Error handling
    if (encInfo->fptr_stego_image == NULL)
    {
    	perror("fopen");
    	fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);

    	return e_failure;
    }

    // No failure return e_success
    return e_success;
}
Status read_and_validate_encode_args(char *argv[],EncodeInfo *encInfo)
{
    //validate .bmp file
   if(argv[2] !=NULL && strcmp(strstr(argv[2], "."), ".bmp") ==0)
   {
    encInfo->src_image_fname = argv[2];
   }
   else
   {
    printf("Error: Invalid or missing .bmp file\n");
    return e_failure;
   }

    //validate .txt file
   if(argv[3] !=NULL && strcmp(strstr(argv[3], "."), ".txt") ==0)
   {
    encInfo->secret_fname = argv[3];
   }
   else
   {
    printf("Error: Invalid or missing .txt file\n");
    return e_failure;
   }

   //optional file --stego.bmp
   if(argv[4] !=NULL)
    {
        encInfo->stego_image_fname =argv[4];
    }
    else
    {
        encInfo->stego_image_fname = "stego.bmp";
    }
    return e_success;

}
Status get_file_size(FILE *fptr_secret)
{
    fseek(fptr_secret,0,SEEK_END);
    return ftell(fptr_secret);
}
Status check_capacity(EncodeInfo *encInfo)
{
    //get the .bmp image file size
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);
    if(encInfo->image_capacity > (54 + (( 2 + 4 + 4 + 4 +encInfo->size_secret_file) *8)))
    {
        return e_success;
    }
    else
    {
        return e_failure;
    }
}
Status copy_bmp_header(FILE *fptr_src, FILE *fptr_stego)
{
    char header[54];
    //go with the fread because .bmp is image data
    fseek(fptr_src,0,SEEK_SET);
    fread(header,54,sizeof(char),fptr_src);
    fwrite(header,sizeof(char),54,fptr_stego);
    return e_success;
}
Status encode_byte_to_lsb(char data, char *image_buffer)
{
    unsigned char mask = 1 << 7;
    //loop for 8 bits
    for(int i=0;i<8;i++)
    {
      image_buffer[i] = (image_buffer[i] & 0xFE) | ((data & mask) >> (7 - i));
        mask = mask >> 1;
    }
    return e_success;
}
Status encode_data_to_image(const char *data,int size, FILE *fptr_src_image, FILE *fptr_stego_image, EncodeInfo *encInfo)
{
  for(int i=0;i<size;i++)
  {
    //read 8 bytes of RGB from source image
   fread(encInfo->image_data,8,sizeof(char),fptr_src_image);
   //call encode_byte_to_lsb function
    encode_byte_to_lsb(data[i],encInfo->image_data);
    //after encoding the data will be innhte same array, write that into destination image
    fwrite(encInfo->image_data,sizeof(char),8,fptr_stego_image);
  }
    return e_success;
}
Status encode_magic_string(const char *magic_str, EncodeInfo *encInfo)
{
    encode_data_to_image(magic_str, strlen(MAGIC_STRING), encInfo->fptr_src_image, encInfo->fptr_stego_image, encInfo);    
    return e_success;
}
Status encode_size_to_lsb(char *image_buffer, int size)
{
    unsigned int mask = 1 << 31;
    //loop for 8 bits
    for(int i=0;i<32;i++)
    {
      image_buffer[i] = (image_buffer[i] & 0xFE) | ((size & mask) >> (31- i));
        mask = mask >> 1;
    }
    return e_success;
}

Status encode_size(int size, FILE *fptr_src_image, FILE *fptr_stego_image)
{
    char str[32];
    fread(str,32,sizeof(char),fptr_src_image);
    //call encode_size_to_lsb function
    encode_size_to_lsb(str,size);
    fwrite(str,sizeof(char),32,fptr_stego_image);
    return e_success;
}
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    file_extn=".txt";
    encode_data_to_image(file_extn, strlen(file_extn), encInfo->fptr_src_image, encInfo->fptr_stego_image, encInfo);
    return e_success;
}
Status encode_secret_file_size(long int file_size, EncodeInfo *encInfo)
{
    char str[32];
    fread(str,32,sizeof(char),encInfo->fptr_src_image);
    //call encode_size_to_lsb function
    encode_size_to_lsb(str,file_size);
    fwrite(str,sizeof(char),32,encInfo->fptr_stego_image);
    return e_success;
}
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    char ch;
    //bring the file pointer to the beginning of secret file
    fseek(encInfo->fptr_secret, 0 ,SEEK_SET);
    for(int i=0;i<encInfo->size_secret_file;i++)
    {
        fread(encInfo->image_data,8,sizeof(char), encInfo->fptr_src_image);
        //read a character from secret file
        fread(&ch,1,sizeof(char), encInfo->fptr_secret);
        encode_byte_to_lsb(ch,encInfo->image_data);
        fwrite(encInfo->image_data,8,sizeof(char), encInfo->fptr_stego_image);
    }
    return e_success;

}
Status copy_remaining_image_data(FILE *fptr_src, FILE *fptr_stego)
{
    char ch;
  while(fread(&ch,1,1,fptr_src) > 0)
  {
    fwrite(&ch,1,1, fptr_stego);
  }
  return e_success;
}
Status do_encoding(EncodeInfo *encInfo)
{
    //calling the rest of the encoding function
    if(open_files(encInfo)== e_success)
    {
        printf("Successfully opened all the required files\n");
        if(check_capacity(encInfo) == e_success)
        {
            printf("Image has enough RGB data to encode.. u can proceed\n");
            if(copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_success)
            {
                printf("Copied the Header successfully\n");
                if(encode_magic_string(MAGIC_STRING, encInfo)==e_success)
                {
                    printf("Successfully encoded the magic string\n");
                    if(encode_size(strlen(".txt"), encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_success)
                    {
                     printf("Successfully encoded the secret file extension size\n");
                     if(encode_secret_file_extn(encInfo->extn_secret_file, encInfo)== e_success)
                     {
                        printf("Successfully encoded the secret file extension\n");
                        if(encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_success)
                        {
                            printf("Successfully encoded the secret file size\n");
                            if(encode_secret_file_data(encInfo) == e_success)
                            {
                                printf("Encoded the secret data successfully\n");
                                if(copy_remaining_image_data(encInfo->fptr_src_image, encInfo->fptr_stego_image)== e_success)
                                {
                                    printf("Copied remaining RGB data successfully\n");
                                }
                                else
                                {
                                 printf("Failed to copy remaining RGB data\n");
                                 return e_failure;
                                }
                            }
                            else
                            {
                                printf("Failed to encode the secret data\n");
                                return e_failure;
                            }
                        }
                        else
                        {
                            printf("Failed to encode the secret file size\n");
                            return e_failure;
                        }
                     }
                     else
                     {
                        printf("Failed to encode the secret file extension\n");
                        return e_failure;
                     }
                    }
                    else
                    {
                        printf("Failed to encode the secret file extension size\n");
                        return e_failure;
                    }
    
                }
                else
                {
                    printf("Failed to encode the magic string\n");
                    return e_failure;
                }
            }
            else
            {
               printf("Failed to copy the headers\n");
                return e_failure; 
            }
        }
        else
        {
            printf("Image doesn't have enough RGB data to encode\n");
            return e_failure;
        }
    }
    else
    {
      printf("Failed to open files\n");
        return e_failure;
    }
    return e_success;
}
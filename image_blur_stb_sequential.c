/**
* @file image_blur_gray.c
* @brief C program to blur the grayscale image, using convolution
* @author Priya Shah
* @version v1
* @date 2018-01-10
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MAX_SIZE 200000

int main(int argc, char** argv)
{

    int channels = 3;
    int k_size = atoi(argv[3]);

    struct dirent *dp;
    DIR *dfd;

    char *dir ;
    dir = argv[1] ;

    if ((dfd = opendir(dir)) == NULL)
    {
        fprintf(stderr, "Can't open %s\n", dir);
        return 0;
    }

    char filename_qfd[100] ;
    char dot1[100];
    char dot2[100];
    strcat(strcpy(dot1, dir),"/.");
    strcat(strcpy(dot2, dir),"/..");

    int photo_counter = 1;
    //loop dir
    while ((dp = readdir(dfd)) != NULL)
    {
        sprintf( filename_qfd , "%s/%s",dir,dp->d_name) ;

        //skip /.. and /.
        if(!strcmp(filename_qfd,dot1) || !strcmp(filename_qfd,dot2) ) continue;

        //blur
        int width, height, bpp;
        uint8_t* data = stbi_load(filename_qfd, &width, &height, &bpp, channels);

        //cod
        int i,j;

        int size = height*width;									//calculate image size

        //uint8_t out[size*channels];						//to store the image data
        uint8_t *out = (uint8_t*) malloc(size*channels*sizeof(uint8_t));

        //char out_name[100];
        //strcpy(out_name, "teste.jpg");
        //stbi_write_jpg(out_name,  received->width, received->height, channels, received->data, 100);

        //kernel
        int offset = k_size / 2;
        float v= 1.0 / (float) (k_size*k_size);

        //blur
        for(int x=offset;x<height-offset;x++) {
            for(int y=offset;y<width-offset;y++) {
                for(int c=0; c < channels; c++) {
                    float sum= 0.0;
                    for(i=-offset;i<=offset;++i) {
                        for(j=-offset;j<=offset;++j) {
                            uint8_t* pixel = data + ( (y+j) + (x+i) * width) * channels;
                            sum += pixel[c] * v;
                        }
                    }
                    uint8_t* pixel = out + ( y + x * width) * channels;
                    pixel[c] = (int) sum;

                }
            }
        }

        //save image
        char out_name[100];
        char num_file[5];
        strcpy(out_name, argv[2]);
        strcat(out_name, "/foto");
        snprintf(num_file,5, "%d",photo_counter);
        strcat(out_name,num_file);
        strcat(out_name,".jpg");

        stbi_write_jpg(out_name,  width, height, channels, out, 100);
        free(out);
        stbi_image_free(data);
        photo_counter++;


    }

	return 0;
}


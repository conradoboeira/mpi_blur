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
#include "mpi.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MAX_SIZE 200000

typedef struct{
    uint8_t data[MAX_SIZE*3];
    int height;
    int width;
}image;

int main(int argc, char** argv)
{

    int my_rank;
    int proc_n;
    int ntag = 50; //normal tag
    int ktag = 66; //kill tag

    MPI_Status status;
    MPI_Init(&argc, & argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_n);

    int channels = 3;
    int k_size = atoi(argv[3]);

	//DEFINE MPI STRUCTURE
    const int nitems=3;
    int          blocklengths[3] = {MAX_SIZE*3,1,1};
    MPI_Datatype types[3] = {MPI_UINT8_T, MPI_INT, MPI_INT};
    MPI_Datatype mpi_image_type;
    MPI_Aint     offsets[3];

    offsets[0] = offsetof(image, data);
    offsets[1] = offsetof(image, height);
    offsets[2] = offsetof(image, width);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_image_type);
    MPI_Type_commit(&mpi_image_type);


    //MASTER
    if(my_rank == 0){
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

        int index = 1;
        int received = 0;

        while ((dp = readdir(dfd)) != NULL)
        {
            sprintf( filename_qfd , "%s/%s",dir,dp->d_name) ;	
            
            if(!strcmp(filename_qfd,dot1) || !strcmp(filename_qfd,dot2) ) continue;

            // first dash
            if(index < proc_n){
                int width, height, bpp;
                uint8_t* data = stbi_load(filename_qfd, &width, &height, &bpp, channels);
				image* to_send = malloc(sizeof(image));
				memcpy(to_send->data, data, sizeof(to_send->data));
				to_send->width = width;
				to_send->height = height;
                MPI_Send(to_send, 1, mpi_image_type,index, ntag, MPI_COMM_WORLD);
                stbi_image_free(data);
                free(to_send);
            } 
            
            // waiting
            else{
				image* out = malloc(sizeof(image));
                MPI_Recv(out, 1, mpi_image_type, MPI_ANY_SOURCE, ntag, MPI_COMM_WORLD, &status);
                received ++;

                char out_name[100];
                char num_file[5];
                strcpy(out_name, argv[2]);
                strcat(out_name, "/foto");
                snprintf(num_file,5, "%d",received);
                strcat(out_name,num_file);
                strcat(out_name,".jpg");
                stbi_write_jpg(out_name,  out->width, out->height, channels, out->data, 100);


                int width, height, bpp;
                uint8_t* data = stbi_load(filename_qfd, &width, &height, &bpp, channels);
				image* to_send = malloc(sizeof(image));
				memcpy(to_send->data, data, sizeof(to_send->data));
				to_send->width = width;
				to_send->height = height;
                MPI_Send(to_send,1, mpi_image_type,status.MPI_SOURCE, ntag, MPI_COMM_WORLD);
                stbi_image_free(data);
                free(to_send);
                free(out);
            }

            index++;

        }

        while(received < index-1){
			image* out = malloc(sizeof(image));

			MPI_Recv(out, 1, mpi_image_type, MPI_ANY_SOURCE, ntag, MPI_COMM_WORLD, &status);
            received ++;

            char out_name[100];
            char num_file[5];
            strcpy(out_name, argv[2]);
            strcat(out_name, "/foto");
            snprintf(num_file,5, "%d",received);
            strcat(out_name,num_file);
            strcat(out_name,".jpg");
            stbi_write_jpg(out_name, out->width, out->height, channels, out->data, 100);
            free(out);
        }

        for(int i = 1; i < proc_n;i++){
			image* dummy= malloc(sizeof(image));
            MPI_Send(dummy, 1, mpi_image_type, i, ktag, MPI_COMM_WORLD);
            free(dummy);
        }

    }
    
    //SLAVE
    else{
        while(1){
            image* received = malloc(sizeof(image));
            MPI_Recv(received, 1, mpi_image_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            
            if(status.MPI_TAG == ktag) {
                free(received);
                break;
            }
            
            
            //uint8_t* data = received.data;
            int height = received->height;
            int width = received->width;
            uint8_t *data = (uint8_t*) malloc(height*width*channels*sizeof(uint8_t));
            memcpy(data, received->data, height*width*channels);
            
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

            float** kernel = (float**)malloc(k_size * sizeof(float*));
            for (int i = 0; i < k_size; i++) kernel[i] = (float*)malloc(k_size * sizeof(float));
            for (int i = 0; i < k_size; i++)
                for (int j = 0; j < k_size; j++)
                    kernel[i][j] = v;

            //blur
            for(int x=offset;x<height-offset;x++)
            {
                for(int y=offset;y<width-offset;y++)
                {
                    for(int c=0; c < channels; c++) {
                        float sum= 0.0;
                        for(i=-offset;i<=offset;++i)
                            {
                                for(j=-offset;j<=offset;++j)
                                    {
                                        uint8_t* pixel = data + ( (y+j) + (x+i) * width) * channels;
                                        sum += pixel[c] * (float)kernel[i+offset][j+offset];	//matrix multiplication with kernel
                                    }
                            }
                        uint8_t* pixel = out + ( y + x * width) * channels;
                        pixel[c] = (int) sum;

                    }
                }
            }

            image* to_send = malloc(sizeof(image));
            memcpy(to_send->data, out, size*channels);
            to_send->width = width;
            to_send->height = height;
            MPI_Send(to_send,1, mpi_image_type,status.MPI_SOURCE, ntag, MPI_COMM_WORLD);
            free(data);
            free(out);
            free(to_send);
            free(received);
        }

	}

    MPI_Finalize();
	return 0;
}


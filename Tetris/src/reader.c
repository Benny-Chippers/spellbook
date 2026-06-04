#include "bmp_handler.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
    FILE *fptr;
    bmp paddle = read_bmp("./include/sprites/paddle.bmp");
    bmp background = read_bmp("./include/sprites/background.bmp");
    bmp ball = read_bmp("./include/sprites/ball.bmp");

    fptr = fopen("palette.txt", "w");
    for(int i = 0; i < sizeof(background.palette)/sizeof(background.palette[0]); i++){
        fprintf(fptr, "%d,", background.palette[i].red);
        fprintf(fptr, "%d,", background.palette[i].green);
        fprintf(fptr, "%d,\n", background.palette[i].blue);
    }
    fclose(fptr);

    fptr = fopen("background_pixels.txt", "w");
    int counter = 0;
    for(int i = 0; i < background.dib_header.width * background.dib_header.height; i++){
        counter++;
        fprintf(fptr, "%d,", background.pixels[i]);
        if(counter == 255)
        {
            counter = 0;
            fprintf(fptr, "\n");
        }
    }
    fclose(fptr);



}
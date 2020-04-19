#include <stdint.h>
#include <vector>

#include "img_converter.h"

int main() {
    std::vector<int> ROIrow = {0,8000};
    std::vector<int> ROIcol = {0,1200}; //all

    uint8_t threshold = 200;


    // READ FILE
    int width; 
    int height;
    int bpp;

/*
    uint8_t* rgb_image_loaded = stbi_load("../img/vid00076.bmp", &width, &height, &bpp, 3);

    // WRITE FILE

//    uint8_t* rgb_image;
//    rgb_image = (uint8_t*) malloc(width*height*CHANNEL_NUM);
    for (int i = 0; i < width*height*CHANNEL_NUM-999; i+=CHANNEL_NUM) {
        int col = (i/CHANNEL_NUM)%width;
        int row = i/CHANNEL_NUM/width;
        if (row > ROIrow[0] && row < ROIrow[1] && col > ROIcol[0] && col < ROIcol[1]) {
            uint8_t bw = rgb2bw(rgb_image_loaded[i],rgb_image_loaded[i+1],rgb_image_loaded[i+2],threshold);
            rgb_image_loaded[i] = bw;
            rgb_image_loaded[i+1] = bw;
            rgb_image_loaded[i+2] = bw;
        }
    }
    // Write your code to populate rgb_image here

    stbi_write_png("../img/image.png", width, height, CHANNEL_NUM, rgb_image_loaded, width*CHANNEL_NUM);


    stbi_image_free(rgb_image_loaded);

*/
    return 0;
}
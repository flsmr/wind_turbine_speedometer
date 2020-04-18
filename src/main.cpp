#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define CHANNEL_NUM 3

int main() {
    // WRITE FILE
    int width = 800; 
    int height = 800;

    uint8_t* rgb_image;
    rgb_image = (uint8_t*) malloc(width*height*CHANNEL_NUM);

    // Write your code to populate rgb_image here

    stbi_write_png("../img/image.png", width, height, CHANNEL_NUM, rgb_image, width*CHANNEL_NUM);

    // READ FILE
    //int width, height, 
    int bpp;

    uint8_t* rgb_image_loaded = stbi_load("../img/image.png", &width, &height, &bpp, 3);

    stbi_image_free(rgb_image_loaded);


    return 0;
}
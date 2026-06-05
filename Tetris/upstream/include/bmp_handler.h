#ifndef BMP_HANDLER_H
#define BMP_HANDLER_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// Header file is defined per BMP standard
// https://en.wikipedia.org/wiki/BMP_file_format

#pragma pack(push, 1)
typedef struct {
    uint16_t header_signature;
    uint32_t header_size;
    uint16_t reserved_1, reserved_2;
    uint32_t offset;
} bmp_file_header;

typedef struct {
    uint32_t header_size;
    uint32_t width;
    uint32_t height;
    uint16_t num_color_plane;
    uint16_t pixel_bit_count;
    uint32_t compression_method;
    uint32_t image_size;
    uint32_t image_horizontal_res;
    uint32_t image_vertical_res;
    uint32_t palette_color_num;
    uint32_t important_colors;
} bmp_dib_header;
#pragma pack(pop)

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
} rgb_quad;

typedef struct {
    bmp_file_header header;
    bmp_dib_header dib_header;
    rgb_quad palette[256];
    uint8_t *pixels;
} bmp;

bmp read_bmp(const char* filename);

#endif

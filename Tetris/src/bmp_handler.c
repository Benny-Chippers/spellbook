#include "bmp_handler.h"

bmp read_bmp(const char* filename)
{
    bmp our_bmp, empty = {0};
    FILE *bmp_raw = fopen(filename, "rb");

    // Ensure proper file
    if(!bmp_raw){ 
        printf("Error: Could not open file %s\n", filename); 
        return empty;
    }

    // Read the bmp_file__header
    if(fread(&our_bmp.header, sizeof(bmp_file_header), 1, bmp_raw) != 1){
        printf("Error: Could not read file header\n");
        fclose(bmp_raw);
        return empty;
    }
    // Read the bmp_dib_Header
    if(fread(&our_bmp.dib_header, sizeof(bmp_dib_header), 1, bmp_raw) != 1){
        printf("Error: Could not read dib_header\n");
        fclose(bmp_raw);
        return empty;
    }

    // Ensure the bit depth is 4
    if(our_bmp.dib_header.pixel_bit_count != 8) {
        printf("Error: Only 4-bit color depth is supported\n"); 
        fclose(bmp_raw); 
        return empty;
    }

    // Ensure proper dimensions

    // Read the color pallete
    if(fread(&our_bmp.palette, sizeof(rgb_quad), 256, bmp_raw) !=256)
    {
        printf("Error: Could not read the 16-color palette\n");
        fclose(bmp_raw);
        return empty;
    }

    // Dynamic Memory Allocation for Pixels
    // Calculate bytes needed: (Width * Height) / 2 (since 2 pixels per byte)
    // Note: This logic assumes your image width is an even number to avoid padding issues.
    // May want to fix the odd number of pixels in the future
    uint32_t row_size = ((our_bmp.dib_header.width + 3) / 4) * 4;
    uint32_t data_size = row_size * our_bmp.dib_header.height;

    our_bmp.pixels = (uint8_t*)malloc(data_size);
    if(our_bmp.pixels == NULL) {
        printf("Error: Memory allocation failed\n");
        fclose(bmp_raw);
        return empty;
    }

    // Seek to pixel data (by offset) and read
    fseek(bmp_raw, our_bmp.header.offset, SEEK_SET);
    if(fread(our_bmp.pixels, 1, data_size, bmp_raw) != data_size) {
        printf("Error: Could not read pixel data\n");
        free(our_bmp.pixels);
        fclose(bmp_raw);
        return empty;
    }

    // Close datastream and return the BMP struct
    fclose(bmp_raw);
    return our_bmp;
}
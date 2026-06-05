#include "painter.h"
#include "physics.h"
#define TRANSPARENT_COLOR 166

// Frame sorting

int compare_layers(const void *a, const void *b){

    const actor *s1 = (const actor *)a;
    const actor *s2 = (const actor *)b;

    return(s1->layer - s2->layer);
};

void sort_frame(frame *f, int count){
    qsort(
        f->actors,
        count,
        sizeof(actor),
        compare_layers
    );
};

// Need to implement direciton properly

void rotate_actor(frame *cur_frame, actor *cur_actor, int rotation_direction){
    
    // Start by cleaning before we rotate

    clean_smear(cur_frame, cur_actor);

    // We want to make a temp actor that we'll then copy over to our current actor
    
    actor temp_actor;

    temp_actor.width = cur_actor->height;
    temp_actor.height = cur_actor->width;
    
    // We then want to copy over the rotated sprites

    for(int x = 0; x < cur_actor->width; x++){
        for(int y = 0; y < cur_actor->height; y++){

            int s_idx = x + y * cur_actor->width;

            temp_actor.sprites[x+y] = cur_actor->sprites[s_idx];

        }
    }

    // Copy over the corrected information
    
    cur_actor->width = temp_actor.width;
    cur_actor->height = temp_actor.height;

    for(int y = 0; y < cur_actor->height; y++){
        for(int x = 0; x < cur_actor->width; x++){

            cur_actor->sprites[x+y] = temp_actor.sprites[x+y];

        }
    }

    return;



}

// Function for cleaning smears
// Currently broken as of 5/30/26

void clean_smear(frame *cur_frame, actor *cur_actor) {

    // Start by getting the maximum height and width of the frame for reference

    uint8_t maxw = cur_frame->maxw;
    uint8_t maxh = cur_frame->maxh;
    uint8_t spritew = cur_actor->width;
    uint8_t spriteh = cur_actor->height;

    // Calculate the delta values so we know whta we need to clean
    // TO IMPLEMENT: if delta values are larger than the width/height of the sprite we only need to clean the previous position plus the maximum width/height

    int delta_x = cur_actor->x_pos - cur_actor->x_prev;
    int delta_y = cur_actor->y_pos - cur_actor->y_prev;

    // First check if the delta values are zero or not

    if (delta_x != 0 || delta_y != 0) {

        // If they aren't zero then we calculate the dynamic smear cleaning

        // The general idea here was to get the minimum and maximum raw values and sweep across those
        // This would account for negatives, but it needs some work because it currently doesn't properly
        
        int min_x_raw = (cur_actor->x_prev < cur_actor->x_pos) ? cur_actor->x_prev : cur_actor->x_pos;
        int min_y_raw = (cur_actor->y_prev < cur_actor->y_pos) ? cur_actor->y_prev : cur_actor->y_pos;
        
        int max_x_raw = (cur_actor->x_prev > cur_actor->x_pos) ? cur_actor->x_prev : cur_actor->x_pos;
        int max_y_raw = (cur_actor->y_prev > cur_actor->y_pos) ? cur_actor->y_prev : cur_actor->y_pos;

        int old_x1 = min_x_raw - 1;
        int old_y1 = min_y_raw - 1;
        int old_x2 = max_x_raw + spritew + 1;
        int old_y2 = max_y_raw + spriteh + 1;

        int min_x = (old_x1 < 0) ? 0 : old_x1;
        int min_y = (old_y1 < 0) ? 0 : old_y1;
        int max_x = (old_x2 >= maxw) ? maxw - 1 : old_x2;
        int max_y = (old_y2 >= maxh) ? maxh - 1 : old_y2;

        for (int y = min_y; y <= max_y; y++) {
            for (int x = min_x; x <= max_x; x++) {
            
                if (x >= cur_actor->x_pos && x < cur_actor->x_pos + spritew &&
                    y >= cur_actor->y_pos && y < cur_actor->y_pos + spriteh) {
                    continue;
                }

                int f_idx = x + y * maxw;
                cur_frame->pixels[f_idx] = cur_frame->background[f_idx];
            }
        }
    }


    // Handles cleaning up the area where the sprite is (if it changes the active sprite and has transparents)

    else if (cur_actor->layer != 0) {

        // Sweep across all pixels of the sprite

        for (int y = 0; y < spriteh; y++) {
            for (int x = 0; x < spritew; x++) {

                // Target position is the frame position scaled version

                uint8_t target_x = x + cur_actor->x_pos;
                uint8_t target_y = y + cur_actor->y_pos;

                // If parts of the sprite are outside of our maximum then don't clean it or seg fault

                if (target_x < 0 || target_x >= maxw || target_y < 0 || target_y >= maxh) {
                    continue;
                }

                // f_idx refers to the specific pixel in the background

                int f_idx = target_y * maxw + target_x;
                cur_frame->pixels[f_idx] = cur_frame->background[f_idx];
            }
        }
    }

    // This handles changing the active sprite. after the smear of the previous sprite is cleaned
    // you can choose a new sprite. You can only have 254 sprites per actor

    if (cur_actor->to_be_active_sprite != 255) {
        cur_actor->current_sprite = cur_actor->to_be_active_sprite;
        cur_actor->to_be_active_sprite = 255;
    }
    return;
}

// Function for "drawing" the sprite into the frame buffer

void draw_sprite(frame *cur_frame, actor *cur_actor){

    // If the actor layer is 0 it shouldn't be drawn

    if(cur_actor->layer == 0){
        return;
    };

    // Calculate the maximums and set the previous stuff for smear

    uint8_t maxw = cur_frame->maxw;
    uint8_t maxh = cur_frame->maxh;

    cur_actor->x_prev2 = cur_actor->x_prev;
    cur_actor->y_prev2 = cur_actor->y_prev;
    cur_actor->x_prev = cur_actor->x_pos;
    cur_actor->y_prev = cur_actor->y_pos;

    // Go through all the pixels, put them into the frame buffer

    for(uint8_t y = 0; y < cur_actor->height; y++){
        for(uint8_t x = 0; x < cur_actor->width; x++){

            // Get the target positions

            int target_x = x + cur_actor->x_pos;
            int target_y = y + cur_actor->y_pos;

            // If they're outside of the frame don't render them

            if(target_x < 0 || target_x >= maxw || target_y < 0 || target_y >= maxh){
                continue;
            }

            // Get the specific row size acocunting for the padding

            uint32_t row_size = ((cur_actor->width + 3) / 4) * 4;

            // Find the proper index and move the colors over the the frame buffer

            int s_idx = x + (y * (row_size));
            int f_idx = target_y * maxw + target_x;
            int color = cur_actor->sprites[cur_actor->current_sprite][s_idx];


            // If it's transparent don't move it

            if(color == TRANSPARENT_COLOR){
                continue;
            }

            cur_frame->pixels[f_idx] = color;


        }

    }
};



// Basic AABB Collision check, may move to physics

int check_collision(actor *actor_a, actor *actor_b){
    int a_left   = actor_a->x_pos;
    int a_right  = a_left + actor_a->width;
    int a_top    = actor_a->y_pos;
    int a_bottom = a_top + actor_a->height;

    int b_left   = actor_b->x_pos;
    int b_right  = b_left + actor_b->width;
    int b_top    = actor_b->y_pos;
    int b_bottom = b_top + actor_b->height;


    if (a_right < b_left ||  // A is to the left of B
        a_left > b_right ||  // A is to the right of B
        a_bottom < b_top ||  // A is above B
        a_top > b_bottom) {  // A is below B
        return 0; 
    }

    return 1;
};

// Funciton for running all of the above functions together

frame *render_frame(frame *cur_frame){

    if(cur_frame->num_layers != 1){
    sort_frame(cur_frame, cur_frame->num_actors);

    ;}



    for(uint8_t i = 0; i < cur_frame->num_actors; i++){
        clean_smear(cur_frame, &cur_frame->actors[i]);
    };

    for(uint8_t i = 0; i < cur_frame->num_actors; i++){
        draw_sprite(cur_frame, &cur_frame->actors[i]);
    };

    return cur_frame;

};

// Below is all constructors and destructors 

frame construct_frame(frame *cur_frame, uint8_t *background, uint8_t num_actors, uint8_t num_layers, uint8_t maxw, uint8_t maxh, uint8_t frame_width, uint8_t frame_height){
    cur_frame->num_actors = num_actors;
    cur_frame->num_layers = num_layers;
    cur_frame->background = background;
    cur_frame->num_actors = num_actors;
    cur_frame->num_layers = num_layers;
    cur_frame->maxw = maxw;
    cur_frame->maxh = maxh;
    cur_frame->frame_width = frame_width;
    cur_frame->frame_height = frame_height;
    uint32_t total_pixels = frame_width * frame_height;
    cur_frame->actors = malloc(sizeof(actor)*num_actors);
    cur_frame->pixels = malloc(total_pixels);
    memcpy(cur_frame->pixels, background, total_pixels);
    return *cur_frame;
};

void destruct_frame(frame *cur_frame){


    if (cur_frame->pixels) {
        free(cur_frame->pixels);
        cur_frame->pixels = NULL;
    }



    cur_frame->background = NULL; 
    cur_frame->actors = NULL;
}

actor construct_actor(actor *cur_actor, uint8_t num_sprites, uint8_t width, uint8_t height,uint8_t layer, uint8_t colission_layer, uint8_t x_pos, uint8_t y_pos, uint8_t *image){
    cur_actor->num_sprites = num_sprites;
    cur_actor->layer = layer;
    cur_actor->collission_layer = colission_layer;
    cur_actor->x_pos = x_pos;
    cur_actor->y_pos = y_pos;
    cur_actor->x_prev = x_pos;
    cur_actor->y_prev = y_pos;
    cur_actor->x_prev2 = x_pos; 
    cur_actor->y_prev2 = y_pos;
    cur_actor->width = width;
    cur_actor->height = height;
    cur_actor->current_sprite = 0;
    cur_actor->to_be_active_sprite = 255;
    cur_actor->sprites = malloc(num_sprites * sizeof(uint8_t *));
    cur_actor->sprites[0] = image;
    cur_actor->physics = malloc(sizeof(physics_obj));
    return *cur_actor;
};

void destruct_actor(actor *cur_actor){


    cur_actor->sprites = NULL;
    cur_actor->layer = 0;
    cur_actor->collission_layer = 0;
}

void write_pixel(uint8_t x, uint8_t y, uint8_t color_idx){
   //vga_write_index_fast(x,y,color_idx);
}

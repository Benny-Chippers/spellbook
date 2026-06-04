#ifndef PAINTER_H
#define PAINTER_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bmp_handler.h"

// IMPORTANT NOTES:
// LAYER 0 AND COLLISSION LAYER 0 ARE RESERVED AS NON_LAYERS
// ACTORS ON LAYER 0 WILL NOT BE RENDERED
// ACTORS WITH COLLISSION LAYER 0 WILL NOT INTERACT WITH OTHER
// TO PREVENT SMEAR YOU SHOULD ONLY CHANGE THE ACTIVE SPRITE AFTER CLEANING THE BACKGROUND

struct physics_obj;

typedef struct actor{
    uint8_t **sprites; // Double pointer for the sprites
    uint8_t width, height;
    uint8_t num_sprites;
    uint8_t current_sprite; // This is the currently active sprite (should be treated as a private variable)
    uint8_t to_be_active_sprite; // This sets the sprite that you want to be active
    uint8_t layer, collission_layer; 
    uint8_t x_pos, y_pos;
    uint8_t x_prev, y_prev, x_prev2, y_prev2; // Important for the smear
    struct physics_obj *physics; // This object controls the actor physics
} actor;


typedef struct{
    bmp *background;
    actor *actors;
    uint8_t maxw, maxh, frame_height, frame_width;
    uint8_t num_actors, num_layers;
    uint8_t *pixels;
} frame;


int compare_layers(const void *a, const void *b);
void sort_frame(frame *f, int count);
void rotate_actor(frame *cur_frame, actor *cur_actor, int rotation_direction);
void clean_smear(frame *cur_frame, actor *cur_sprite);
void draw_sprite(frame *cur_frame, actor *cur_sprite);
int check_collision(actor *actor_a, actor *actor_b);
frame *render_frame(frame *cur_frame);
frame construct_frame(frame *cur_frame, uint8_t *background, uint8_t num_actors, uint8_t num_layers, uint8_t maxw, uint8_t maxh, uint8_t frame_width, uint8_t frame_height);
void destruct_frame(frame *cur_frame);
actor construct_actor(actor *cur_actor, uint8_t num_sprites, uint8_t width, uint8_t height, uint8_t layer, uint8_t colission_layer, uint8_t x_pos, uint8_t y_pos, uint8_t *image);
void destruct_actor(actor *cur_actor);
void write_pixel(uint8_t x, uint8_t y, uint8_t color_idx);


#endif
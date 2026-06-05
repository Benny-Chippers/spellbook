#ifndef PHYSICS_H
#define PHYSICS_H
#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<unistd.h>
#include "painter.h"
#include "bmp_handler.h"

struct actor; 
typedef struct physics_obj{
    int max_speed;
    int x_acceleration_magnitude, y_acceleration_magnitude;
    int x_velocity, y_velocity;
    int x_acceleration, y_acceleration;
    int x_rotational_accel, y_rotational_accel;
    int x_friction, y_friction;
    int rotation;
} physics_obj;

void calculate_rotational_velocity(physics_obj *obj);
void calculate_linear_velocity(physics_obj *obj);
void execute_movement(struct actor *cur_actor, physics_obj *obj, int delta_time);
physics_obj *construct_physics_obj(physics_obj *obj, int max_speed, int x_acceleration_magnitude, int y_acceleration_magnitude, int x_friction, int y_friction, int rotation);

#endif

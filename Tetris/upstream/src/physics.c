#include "physics.h"
#include "raylib.h"
#include "painter.h"

void calculate_rotational_velocity(physics_obj *obj){


     if(obj->rotation > 360){
            obj->rotation = obj->rotation- 360;
    }

    if(obj->rotation < 1){
            obj->rotation = 360 + obj->rotation;
    };

    if(IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)){
        obj->x_acceleration = 1;
        obj->y_acceleration = 1;
    }

    else if(IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)){
        obj->x_acceleration = -1;
        obj->y_acceleration = -1;
    }
    else{
        obj->x_acceleration = 0;
        obj->y_acceleration = 0;
    };

    if(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)){
        obj->rotation -= 10;
    }

    if(IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)){
        obj->rotation += 10;
    };

    if(obj->rotation > 337 || obj->rotation < 22){
            obj->y_rotational_accel = 1;
            obj->x_rotational_accel = 0;
    }

    if(obj->rotation > 23 && obj->rotation < 67){
            obj->x_rotational_accel = 1;
            obj->y_rotational_accel = 1;
    }

    if(obj->rotation> 67 && obj->rotation < 112){
            obj->x_rotational_accel = 1;
            obj->y_rotational_accel = 0;
    }

    if(obj->rotation > 112 && obj->rotation < 157){
            obj->x_rotational_accel = 1;
            obj->y_rotational_accel = -1;
    }

    if(obj->rotation > 157 && obj->rotation < 202){
            obj->y_rotational_accel = -1;
            obj->x_rotational_accel = 0;
    }

    if(obj->rotation > 202 && obj->rotation < 247){
            obj->x_rotational_accel = -1;
            obj->y_rotational_accel = -1;
            
    }

    if(obj->rotation > 247 && obj->rotation <292){
            obj->x_rotational_accel = -1;
            obj->y_rotational_accel = 0;
    }

    if(obj->rotation > 292 && obj->rotation < 336){
            obj->x_rotational_accel = -1;
            obj->y_rotational_accel = 1;
    };

    obj->x_acceleration = obj->x_acceleration * obj->x_rotational_accel;
    obj->y_acceleration = obj->y_acceleration * obj->y_rotational_accel;

    return;
};

void calculate_linear_velocity(physics_obj *obj){

    return;
};

void execute_movement(actor* cur_actor, physics_obj *obj, int delta_time){
    int x_friction_accel, y_friction_accel;
    x_friction_accel = 0;
    y_friction_accel = 0;

    if(obj->x_velocity > 0){
        x_friction_accel = obj->x_friction;
    }
    else if(obj->x_velocity < 0){
        x_friction_accel = -obj->x_friction;
    }; 

    if(obj->y_velocity > 0){
        y_friction_accel = obj->y_friction;
    }
    if(obj->y_velocity < 0){
        y_friction_accel = -obj->y_friction;
    };

    int prev_x_vel = obj->x_velocity;
    int prev_y_vel = obj->y_velocity;


    obj->x_velocity = obj->x_velocity + (obj->x_acceleration * obj->x_acceleration_magnitude - x_friction_accel) * delta_time;
    obj->y_velocity = obj->y_velocity + (obj->y_acceleration * obj->y_acceleration_magnitude - y_friction_accel) * delta_time;

    if(obj->x_velocity > obj->max_speed){obj->x_velocity = obj->max_speed;}
    if(obj->x_velocity < -obj->max_speed){obj->x_velocity = -obj->max_speed;};
    if(obj->y_velocity > obj->max_speed){obj->y_velocity = obj->max_speed;}
    if(obj->y_velocity < -obj->max_speed){obj->y_velocity = -obj->max_speed;};

    if(obj->x_acceleration * obj->x_acceleration_magnitude == 0){
        if((prev_x_vel > 0 && obj->x_velocity < 0) || (prev_x_vel < 0 && obj->x_velocity > 0)){
            obj->x_velocity = 0;
        }
    }

    if(obj->y_acceleration * obj->y_acceleration_magnitude == 0){
        if((prev_y_vel > 0 && obj->y_velocity < 0) || (prev_y_vel < 0 && obj->y_velocity > 0)){
            obj->y_velocity = 0;
        }
    }
    cur_actor->x_pos = cur_actor->x_pos + obj->x_velocity * delta_time;
    cur_actor->y_pos = cur_actor->y_pos + obj->y_velocity * delta_time;

    return;
};

physics_obj *construct_physics_obj(physics_obj * obj, int max_speed, int x_acceleration_magnitude, int y_acceleration_magnitude, int x_friction, int y_friction, int rotation){
    obj->max_speed = max_speed;
    obj->x_acceleration_magnitude = x_acceleration_magnitude;
    obj->y_acceleration_magnitude = y_acceleration_magnitude;
    obj->x_friction = x_friction;
    obj->y_friction = y_friction;
    obj->rotation = rotation;
    obj->x_velocity = 0;
    obj->y_velocity = 0;
    obj->x_acceleration = 0;
    obj->y_acceleration = 0;   
    obj->x_rotational_accel = 0;
    obj->y_rotational_accel = 0;
};
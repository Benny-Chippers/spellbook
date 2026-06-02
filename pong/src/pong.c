#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<unistd.h>
#include "painter.h"
#include "bmp_handler.h"
#include "raylib.h"
#include "images.h"

int main(){


    int frameWidth = 160;
    int frameHeight = 120;

    frame MainFrame = construct_frame(&MainFrame, pong_background, 3, 1, frameWidth, frameHeight, frameWidth, frameHeight);
    MainFrame.actors[0] = construct_actor(&(MainFrame.actors[0]), 1, 16, 4, 1, 1, frameWidth/2, frameHeight/6, paddle_pixels);
    actor *paddle_1 = &(MainFrame.actors[0]);
    MainFrame.actors[1] = construct_actor(&(MainFrame.actors[1]), 1, 16, 4, 1, 1, frameWidth/2, frameHeight - frameHeight/6, paddle_pixels);
    actor *paddle_2 = &(MainFrame.actors[1]);
    MainFrame.actors[2] = construct_actor(&(MainFrame.actors[2]),1, 4, 4, 1, 1, frameWidth/2, frameHeight/2, ball_pixels);
    actor *ball = &(MainFrame.actors[2]);
    paddle_1->x_pos = paddle_1->x_pos - paddle_1->width/2;
    paddle_2->x_pos = paddle_2->x_pos - paddle_2->width/2;
    int ball_y_speed = 1;
    int ball_x_speed = 0;
    int win = 60;
    int start = 0;





// RENDERING BULLSHIT

    while(!WindowShouldClose())
    {

        if(start == 0)
        {
            ball_y_speed = 0;
            
            if(IsKeyDown(KEY_S))
            {
                start = 1;
                ball_y_speed = 1;
            }
        }

        if(IsKeyDown(KEY_RIGHT)){
            if(paddle_1->x_pos < 120)
            {
                paddle_1->x_pos = paddle_1->x_pos + 2;
            }
        }
        if(IsKeyDown(KEY_LEFT)){
            if(paddle_1->x_pos > 20)
            {
                paddle_1->x_pos = paddle_1->x_pos - 2;
            }
        }
         if(paddle_2->x_pos < ball->x_pos + 6){
            paddle_2->x_pos = paddle_2->x_pos + 2;
        }
        if(paddle_2->x_pos > ball->x_pos - 6){
            paddle_2->x_pos = paddle_2->x_pos - 2;
        }
        ball->y_pos = ball->y_pos + ball_y_speed;
        ball->x_pos = ball->x_pos + ball_x_speed;


        render_frame(&MainFrame);

        if(check_collision(paddle_2, ball))
        {
            if(ball->x_pos >= paddle_2->x_pos && ball->x_pos <= paddle_2->x_pos + 6)
            {
                ball_y_speed = -1;
                ball_x_speed = -1;
            }
            else if(ball->x_pos <= paddle_2->x_pos + paddle_2->width && ball->x_pos >= paddle_2->x_pos + paddle_2->width - 6)
            {
                ball_y_speed = -1;
                ball_x_speed = 1;
            }
            else
            {
                ball_y_speed = -1;
                ball_x_speed = 0;
            }
        };

        if(check_collision(paddle_1, ball))
        {
            if(ball->x_pos >= paddle_1->x_pos && ball->x_pos <= paddle_1->x_pos + 6)
            {
                ball_y_speed = 1;
                ball_x_speed = -1;
            }
            else if(ball->x_pos <= paddle_1->x_pos + paddle_1->width && ball->x_pos >= paddle_1->x_pos + paddle_1->width - 6)
            {
                ball_y_speed = 1;
                ball_x_speed = 1;
            }
            else
            {
                ball_y_speed = 1;
                ball_x_speed = 0;
            }


        };
        if(ball->x_pos < 20)
        {
            ball_x_speed = 1;
        };
        if(ball->x_pos > 120)
        {
            ball_x_speed = -1;
        };
        if(ball->y_pos < 20)
        {
            ball->x_pos = frameWidth/2;
            ball->y_pos = frameHeight/2;
            ball_x_speed = 0;
            ball_y_speed = -1;
            start = 0;
        };
        if(ball->y_pos > 100)
        {
            ball->x_pos = frameWidth/2;
            ball->y_pos = frameHeight/2;
            ball_x_speed = 0;
            ball_y_speed = 1;
            start = 0;
        };

        }
    return 0;
};
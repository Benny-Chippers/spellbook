#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "painter.h"
#include "bmp_handler.h"
#include "raylib.h"
#include "Macros.h"
#include <time.h>

typedef struct
{
    int size;
    int x;
    int y;
    int grid[4][4];
} falling_piece;

typedef struct
{
    uint16_t collision[20];
    int locations_colors[20][10];
} tetris_board;

// GEEK FOR GEEKS STUFF

// A utility function to swap to integers
void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

// A function to generate a random permutation of arr[]
void randomize(int arr[], int n)
{
    // Use a different seed value so that we don't get same
    // result each time we run this program

    // Start from the last element and swap one by one. We don't
    // need to run for the first element that's why i > 0
    for (int i = n - 1; i > 0; i--)
    {
        // Pick a random index from 0 to i
        int j = rand() % (i + 1);

        // Swap arr[i] with the element at random index
        swap(&arr[i], &arr[j]);
    }
}

void printArray(int arr[], int n)
{
    for (int i = 0; i < n; i++)
        printf("%d ", arr[i]);
    printf("\n");
}

// OTHER STUFF FOR TETRIS

void update_falling_piece(int size, int block[size][size], falling_piece *tetris_piece, actor *cur_actor)
{
    tetris_piece->size = size;
    cur_actor->width = size * 4;
    cur_actor->height = size * 4;

    // 1. Completely reset the 4x4 buffer first
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            tetris_piece->grid[y][x] = 0;
        }
    }

    // 2. Safely copy the piece elements using matching type sizes
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            tetris_piece->grid[y][x] = block[y][x];
        }
    }
}

int check_collision_tetris(int target_x, int target_y, falling_piece *my_piece, uint16_t frame_collision[20])
{

    uint16_t mask = 0;

    for (int i = 0; i < my_piece->size; i++)
    {
        for (int j = 0; j < my_piece->size; j++)
        {
            if (my_piece->grid[i][j])
            {
                int check_y = target_y - my_piece->size + i;
                int check_x = target_x + j;

                if (check_x < 0 || check_x > 9 || check_y < 0)
                    return 1;

                if (frame_collision[check_y] & (1 << check_x))
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

void rotate90(int n, int span, int mat[span][span])
{

    // Consider all cycles one by one
    for (int i = 0; i < n / 2; i++)
    {

        // Consider elements in group of 4 as P1, P2, P3 & P4 in current square
        for (int j = i; j < n - i - 1; j++)
        {

            // Swap elements in clockwise order
            int temp = mat[i][j];
            mat[i][j] = mat[n - 1 - j][i];                 // Move P4 to P1
            mat[n - 1 - j][i] = mat[n - 1 - i][n - 1 - j]; // Move P3 to P4
            mat[n - 1 - i][n - 1 - j] = mat[j][n - 1 - i]; // Move P2 to P3
            mat[j][n - 1 - i] = temp;                      // Move P1 to P2
        }
    }
}

void lock_piece(falling_piece *my_piece, tetris_board *f)
{
    for (int i = 0; i < my_piece->size; i++)
    {
        for (int j = 0; j < my_piece->size; j++)
        {

            if (my_piece->grid[i][j] != 0)
            {

                int row = my_piece->y - my_piece->size + i;
                int col = my_piece->x + j;

                if (row >= 0 && row < 20 && col >= 0 && col < 10)
                {

                    f->collision[row] |= (1 << col);

                    f->locations_colors[row][col] = my_piece->grid[i][j];
                }
            }
        }
    }
}

int clear_full_lines(tetris_board *f)
{
    int lines_cleared = 0;
    for (int i = 0; i < 20; i++)
    {
        if (f->collision[i] == 0x03FF)
        { // 10 bits set
            // Shift everything down
            lines_cleared += 1;
            for (int k = i; k < 19; k++)
            {
                f->collision[k] = f->collision[k + 1];
                for (int c = 0; c < 10; c++)
                    f->locations_colors[k][c] = f->locations_colors[k + 1][c];
            }
            // Reset top
            f->collision[19] = 0;
            for (int c = 0; c < 10; c++)
                f->locations_colors[19][c] = 0;
            i--;
        }
    }
    return lines_cleared;
}

void generate_piece(frame *cur_frame, actor *cur_actor, int grid_num_rows, int grid_size_row, int array_stride, int piece_grid[][array_stride])

{
    clean_smear(cur_frame, cur_actor);

    // Locators for the top left of the specific block we're looking at

    int grid_x = 0;
    int grid_y = 0;
    int start_x = 0;
    int start_y = 0;
    int pixel_idx = 0;

    cur_actor->width = grid_size_row * 4;
    cur_actor->height = grid_num_rows * 4;

    // Because the grids are always squares, they will always have the same grid height and width, hence size

    for (int j = 0; j < grid_num_rows; j++)
    {
        for (int i = 0; i < grid_size_row; i++)
        {
            grid_x = i;
            grid_y = j;

            start_x = grid_x * 4;
            start_y = grid_y * 4;

            for (int y = 0; y < 4; y++)
            {
                for (int x = 0; x < 4; x++)
                {
                    pixel_idx = (start_x + x) + (start_y + y) * 4 * grid_size_row;

                    if (piece_grid[j][i] != 0)
                    {
                        cur_actor->sprites[0][pixel_idx] = BLOCKS[piece_grid[j][i]][x + y * 4];
                    }
                    else
                    {
                        cur_actor->sprites[0][pixel_idx] = 166;
                    }
                }
            }
        }
    }
}

int main()
{

    // VARIABLES FOR ACTORS
    srand(time(NULL) ^ getpid());
    int frame_rate;
    int last_move_time = clock();
    int drop_interval = 500;
    int lines_cleared;
    int counter = 0;
    int score = 0;
    int rand_arr[] = {1, 2, 3, 4, 5, 6, 7};
    for (int i = 1; i < 8; i++)
    {
        rand_arr[i - 1] = i;
    }
    int n = sizeof(rand_arr) / sizeof(rand_arr[0]);
    randomize(rand_arr, n);
    printArray(rand_arr, n);

    uint8_t piece_sprite[256];
    uint8_t (*palette)[3] = Pallete;
    uint8_t *background = Background;
    uint8_t *tetris_game_frame_sprite = Frame;

    falling_piece my_piece = {
        .size = 0,
        .x = 3,
        .y = 19,
        .grid = {0}};

    tetris_board tetris_frame = {
        .collision = {0},
        .locations_colors = {0}};

    uint8_t *piece_grid;

    frame main_frame = construct_frame(&main_frame, background, 2, 2, 160, 120, 160, 120);

    construct_actor(&main_frame.actors[1], 7, 12, 12, 2, 1, 74, 80, piece_sprite);
    actor *piece = &main_frame.actors[1];
    construct_actor(&main_frame.actors[0], 1, 40, 80, 1, 1, 62, 16, tetris_game_frame_sprite);
    actor *tetris_game_frame = &main_frame.actors[0];

    update_falling_piece(3, T_PIECE, &my_piece, piece);

    for (int j = 0; j < 20; j++)
    {
        for (int i = 15; i >= 0; i--)
        {
            uint16_t bit = (tetris_frame.collision[j] >> i) & 1;
            printf("%u ", bit);
        }
        printf("\n");
    }

    generate_piece(&main_frame, piece, my_piece.size, my_piece.size, 4, my_piece.grid);
    generate_piece(&main_frame, tetris_game_frame, 20, 10, 10, tetris_frame.locations_colors);

    for (int i = 0; i < my_piece.size; i++)
    {
        for (int j = 0; j < my_piece.size; j++)
        {
            printf("%d ", my_piece.grid[i][j]);
        }
        printf("\n");
    }

    // RENDERING BULLSHIT

    InitWindow(640, 480, "Kill yourself");
    frame_rate = 60;
    SetTargetFPS(frame_rate);

    Image raylibImage = {
        .data = malloc(main_frame.frame_width * main_frame.frame_height * sizeof(Color)),
        .width = main_frame.frame_width,
        .height = main_frame.frame_height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};

    Color *raylibPixels = (Color *)raylibImage.data;

    Texture2D screenTexture = LoadTextureFromImage(raylibImage);

    int framenum = 0;
    while (!WindowShouldClose())
    {
        clock_t current_time = clock();
        int elapsed = (int)(current_time - last_move_time) * 1000 / CLOCKS_PER_SEC;

        // for(int i  = 0; i < my_piece.size; i ++){
        //     for(int j = 0; j < my_piece.size; j++){
        //         if(my_piece.grid[i][j]){
        //             frame_collision[my_piece.y - my_piece.size + i] |=  1 << my_piece.x + j;
        //         }
        //     }
        //     printf("\n");
        // }
        // printf("\n");

        // for(int j = 19; j >= 0; j--){
        //     for(int i = 0; i < 10; i++){

        //         uint16_t bit = (frame_collision[j] >> i) & 1;
        //         printf("%u ", bit);

        //     }
        //     printf("\n");

        // }

        // printf("\n");
        // for(int i  = 0; i < my_piece.size; i ++){
        //     for(int j = 0; j < my_piece.size; j++){
        //         if(my_piece.grid[i][j]){
        //             frame_collision[my_piece.y - my_piece.size + i] |= 0 << my_piece.x + j;
        //         }
        //     }
        //     printf("\n");
        // }
        // printf("\n");

        // ROTATION HANDLING

        if (IsKeyPressed(KEY_Q))
        {

            rotate90(my_piece.size, 4, my_piece.grid);

            // Test for collision
            if (check_collision_tetris(my_piece.x, my_piece.y, &my_piece, tetris_frame.collision))
            {

                // Try moving one to the right
                if (!check_collision_tetris(my_piece.x + 1, my_piece.y, &my_piece, tetris_frame.collision))
                {
                    my_piece.x += 1;
                    piece->x_pos += 4;
                }
                // Try moving one to the left
                else if (!check_collision_tetris(my_piece.x - 1, my_piece.y, &my_piece, tetris_frame.collision))
                {
                    my_piece.x -= 1;
                    piece->x_pos -= 4;
                }
                // Try moving one up
                else if (!check_collision_tetris(my_piece.x, my_piece.y + 1, &my_piece, tetris_frame.collision))
                {
                    my_piece.y += 1;
                    piece->y_pos += 4;
                }
                else
                {

                    rotate90(my_piece.size, 4, my_piece.grid);
                    rotate90(my_piece.size, 4, my_piece.grid);
                    rotate90(my_piece.size, 4, my_piece.grid);
                }
            }
        }

        if (IsKeyPressed(KEY_E))
        {

            rotate90(my_piece.size, 4, my_piece.grid);
            rotate90(my_piece.size, 4, my_piece.grid);
            rotate90(my_piece.size, 4, my_piece.grid);

            // Test for collision
            if (check_collision_tetris(my_piece.x, my_piece.y, &my_piece, tetris_frame.collision))
            {

                // Try moving one to the right
                if (!check_collision_tetris(my_piece.x + 1, my_piece.y, &my_piece, tetris_frame.collision))
                {
                    my_piece.x += 1;
                    piece->x_pos += 4;
                }
                // Try moving one to the left
                else if (!check_collision_tetris(my_piece.x - 1, my_piece.y, &my_piece, tetris_frame.collision))
                {
                    my_piece.x -= 1;
                    piece->x_pos -= 4;
                }
                // Try moving one up
                else if (!check_collision_tetris(my_piece.x, my_piece.y + 1, &my_piece, tetris_frame.collision))
                {
                    my_piece.y += 1;
                    piece->y_pos += 4;
                }
                else
                {

                    rotate90(my_piece.size, 4, my_piece.grid);
                }
            }
        }

        // HORIZONTAL MOVEMENT

        int dx = 0;
        int dy = 0;

        if (IsKeyPressed(KEY_A))
            dx = -1;
        if (IsKeyPressed(KEY_D))
            dx = 1;

        // Calculate where the piece will be
        int next_x = my_piece.x + dx;

        // Check the collision on this move

        if (!check_collision_tetris(next_x, my_piece.y, &my_piece, tetris_frame.collision))
        {
            my_piece.x = next_x;
            piece->x_pos = piece->x_pos + (dx * 4);
        }

        if (elapsed >= drop_interval)
        {

            last_move_time = current_time;

            if (check_collision_tetris(my_piece.x, my_piece.y - 1, &my_piece, tetris_frame.collision))
            {

                lock_piece(&my_piece, &tetris_frame);
                lines_cleared = (clear_full_lines(&tetris_frame));

                switch (lines_cleared)
                {
                case 1:
                    score += 100;
                    drop_interval -= 5;
                    break;
                case 2:
                    score += 300;
                    drop_interval -= 10;
                    break;
                case 3:
                    score += 500;
                    drop_interval -= 15;
                    break;
                case 4:
                    score += 800;
                    drop_interval -= 20;
                    break;
                }

                printf("%d\n", score);

                my_piece.x = 3;  // Back to center
                my_piece.y = 19; // Back to the top

                piece->x_pos = 74;
                piece->y_pos = 80;

                int shape_id = rand_arr[counter];

                counter += 1;

                if (counter >= 7)
                {
                    counter = 0;
                    for (int i = 1; i < 8; i++)
                    {
                        rand_arr[i - 1] = i;
                    }
                    n = sizeof(rand_arr) / sizeof(rand_arr[0]);
                    randomize(rand_arr, n);
                    printArray(rand_arr, n);
                }

                // Switch based on the ID to update the falling piece
                switch (shape_id)
                {
                case 1:
                    update_falling_piece(3, T_PIECE, &my_piece, piece);

                    break;
                case 2:
                    update_falling_piece(4, I_PIECE, &my_piece, piece);
                    break;
                case 3:
                    update_falling_piece(4, O_PIECE, &my_piece, piece);
                    break;
                case 4:
                    update_falling_piece(3, S_PIECE, &my_piece, piece);
                    break;
                case 5:
                    update_falling_piece(3, Z_PIECE, &my_piece, piece);
                    break;
                case 6:
                    update_falling_piece(3, J_PIECE, &my_piece, piece);
                    break;
                case 7:
                    update_falling_piece(3, L_PIECE, &my_piece, piece);
                    break;
                default:
                    update_falling_piece(3, T_PIECE, &my_piece, piece);
                    break;
                }

                generate_piece(&main_frame, tetris_game_frame, 20, 10, 10, tetris_frame.locations_colors);
            }
            else
            {
                my_piece.y = my_piece.y - 1;
                piece->y_pos = piece->y_pos - 4;
            }
        }

        generate_piece(&main_frame, piece, my_piece.size, my_piece.size, 4, my_piece.grid);

        render_frame(&main_frame);

        for (uint32_t y = 0; y < main_frame.frame_height; y++)
        {
            for (uint32_t x = 0; x < main_frame.frame_width; x++)
            {

                int src_y = (main_frame.frame_height - 1) - y;

                int pixel_idx = y * main_frame.frame_width + x;
                int src_pixel_idx = src_y * main_frame.frame_width + x;
                uint8_t palette_color_idx = main_frame.pixels[pixel_idx];

                uint8_t *palette_color = palette[palette_color_idx];
                raylibPixels[src_pixel_idx] = (Color){
                    .r = palette_color[0],
                    .g = palette_color[1],
                    .b = palette_color[2],
                    .a = 255};
            }
        }

        UpdateTexture(screenTexture, raylibPixels);

        BeginDrawing();
        ClearBackground(BLACK);

        DrawTextureEx(screenTexture, (Vector2){0, 0}, 0.0f, 4.0f, WHITE);
        EndDrawing();
        framenum = framenum + 1;
    }

    UnloadTexture(screenTexture);
    free(raylibImage.data);
    destruct_actor(tetris_game_frame);
    destruct_actor(piece);
    destruct_frame(&main_frame);

    CloseWindow();
    return 0;
};

#include "raylib.h"
#include <stdio.h>

#define WIDTH 800
#define HEIGHT 600
#define TARGET_FPS 30

#define BACKGROUND_COLOR BLACK
#define JELLYFISH_COLOR RAYWHITE

int main(void) {
    InitWindow(WIDTH, HEIGHT, "C Jellyfish");
    SetTargetFPS(TARGET_FPS);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

#include "raylib.h"
#include <math.h>
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

        float t = 0.0f;

        for (int i = 9999; i >= 0; i--) {
            float k = 9.0f * cosf(i * 5.0f) * sinf((float)i);
            float e = cosf(i * 3.0f) * cosf(i * 2.0f) * 9.0f;
            float mag = sqrtf(k * k + e * e);
            float s = sinf(t * 0.5f);
            float d = mag * mag * mag / 1999.0f + 1.5f - s * s * s / 3.0f;
            float c = d / 16.0f - t / 48.0f;
            float p = powf(d, sinf(d * d - t));
            DrawPixelV(
                (Vector2){
                    99.0f * sinf(c) + k * p + 200.0f,
                    99.0f * sinf(c * 4.0f) + e * p + 200.0f,
                },
                JELLYFISH_COLOR
            );
        }
        t += PI / 20.0f;
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

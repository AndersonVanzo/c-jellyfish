#include "raylib.h"
#include <math.h>
#include <stdbool.h>

#define WIDTH 800
#define HEIGHT 600
#define TARGET_FPS 60

#define BACKGROUND_COLOR BLACK
#define TRAIL_FADE_COLOR ((Color){9, 9, 9, 24})
#define GLOW_HALO_COLOR ((Color){40, 90, 120, 255})
#define GLOW_CORE_COLOR RAYWHITE

#define CURVE_SAMPLE_COUNT 1500
#define SUPERSAMPLE_FACTOR 2
#define WORLD_HALF_EXTENT 24.65f
#define VIEW_FILL_RATIO 0.9f
#define ANIMATION_LOOP_PERIOD (96.0f * PI)

typedef struct {
    float curve_x;
    float curve_y;
    float radius_base;
} CurveSample;

CurveSample g_curve_samples[CURVE_SAMPLE_COUNT + 1];
Vector2 g_screen_points[CURVE_SAMPLE_COUNT + 1];
float g_radius_base_mean;

// 1 = centered
// 0 = free swimming
float g_follow_strength = 1.0f;

void precomputeCurveSamples(void) {
    double radius_base_sum = 0.0;
    for (int index = 0; index <= CURVE_SAMPLE_COUNT; index++) {
        double curve_angle = 2.0 * PI * index / CURVE_SAMPLE_COUNT;
        double curve_x = 9.0 * cos(curve_angle * 5.0) * sin(curve_angle);
        double curve_y = cos(curve_angle * 3.0) * cos(curve_angle * 2.0) * 9.0;
        double radius_squared = curve_x * curve_x + curve_y * curve_y;
        double radius_base = radius_squared * sqrt(radius_squared) / 1999.0;

        g_curve_samples[index] = (CurveSample){(float)curve_x, (float)curve_y, (float)radius_base};
        if (index < CURVE_SAMPLE_COUNT) {
            radius_base_sum += radius_base;
        }
    }
    g_radius_base_mean = (float)(radius_base_sum / CURVE_SAMPLE_COUNT);
}

void updateScreenPoints(float animation_time, float view_scale, Vector2 view_origin) {
    const float drift_time = animation_time / 48.f;
    const float bell_wave = sinf(animation_time * 0.5f);
    const float pulse_offset = 1.5f - bell_wave * bell_wave * bell_wave / 3.0f;
    const float anchor_phase = (g_radius_base_mean + pulse_offset) / 16.0f - drift_time;
    const float anchor_x = 99.0f * sinf(anchor_phase) * g_follow_strength;
    const float anchor_y = 99.0f * sinf(anchor_phase * 4.0f) * g_follow_strength;

    for (int index = 0; index <= CURVE_SAMPLE_COUNT; index++) {
        const CurveSample *sample = &g_curve_samples[index];
        float pulse_radius = sample->radius_base + pulse_offset;
        float drift_phase = pulse_radius / 16.0f - drift_time;
        float breath = powf(pulse_radius, sinf(pulse_radius * pulse_radius - animation_time));
        g_screen_points[index].x =
            (99.0f * sinf(drift_phase) - anchor_x + sample->curve_x * breath) * view_scale +
            view_origin.x;
        g_screen_points[index].y =
            (99.0f * sinf(drift_phase * 4.0f) - anchor_y + sample->curve_y * breath) * view_scale +
            view_origin.y;
    }
}

void ensureTrailBuffer(RenderTexture2D *trail_buferr) {
    int buffer_width = GetRenderWidth() * SUPERSAMPLE_FACTOR;
    int buffer_height = GetRenderHeight() * SUPERSAMPLE_FACTOR;

    if (trail_buferr->texture.width == buffer_width &&
        trail_buferr->texture.height == buffer_height) {
        return;
    }
    if (trail_buferr->id != 0) {
        UnloadRenderTexture(*trail_buferr);
    }
    *trail_buferr = LoadRenderTexture(buffer_width, buffer_height);
    SetTextureFilter(trail_buferr->texture, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode(*trail_buferr);
    ClearBackground(BACKGROUND_COLOR);
    EndTextureMode();
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_MSAA_4X_HINT);
    InitWindow(WIDTH, HEIGHT, "C Jellyfish");
    SetTargetFPS(TARGET_FPS);

    precomputeCurveSamples();

    RenderTexture2D trail_buffer = {0};
    float animation_time = 0.0f;

    while (!WindowShouldClose()) {
        animation_time += (PI / 20.0f) * (GetFrameTime() * 30.0f);
        if (animation_time >= ANIMATION_LOOP_PERIOD) {
            animation_time -= ANIMATION_LOOP_PERIOD;
        }

        ensureTrailBuffer(&trail_buffer);

        const float buffer_width = (float)trail_buffer.texture.height;
        const float buffer_height = (float)trail_buffer.texture.height;

        const float pixel_scale = buffer_height / (float)GetScreenHeight();

        const float view_scale = buffer_height * 0.5f * VIEW_FILL_RATIO / WORLD_HALF_EXTENT;
        const Vector2 view_origin = {buffer_width * 0.5f, buffer_height * 0.5f};

        updateScreenPoints(animation_time, view_scale, view_origin);

        BeginTextureMode(trail_buffer);
        DrawRectangle(0, 0, (int)buffer_width, (int)buffer_height, TRAIL_FADE_COLOR);
        BeginBlendMode(BLEND_ADDITIVE);
        DrawSplineLinear(g_screen_points, CURVE_SAMPLE_COUNT + 1, 3.0f * pixel_scale,
                         GLOW_HALO_COLOR);
        DrawSplineLinear(g_screen_points, CURVE_SAMPLE_COUNT + 1, 1.2f * pixel_scale,
                         GLOW_CORE_COLOR);
        EndBlendMode();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        DrawTexturePro(trail_buffer.texture, (Rectangle){0.0f, 0.0f, buffer_width, -buffer_height},
                       (Rectangle){0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight()},
                       (Vector2){0.0f, 0.0f}, 0.0f, RAYWHITE);
        EndDrawing();
    }

    UnloadRenderTexture(trail_buffer);
    CloseWindow();
    return 0;
}

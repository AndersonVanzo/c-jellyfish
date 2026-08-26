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

#define HALO_THICKNESS 0.27f
#define CORE_THICKNESS 0.11f

#define CURVE_SAMPLE_COUNT 1500
#define SUPERSAMPLE_FACTOR 2
#define CENTERED_HALF_EXTENT 24.65f
#define DRIFT_AMPLITUDE 99.0f
#define VIEW_FILL_RATIO 0.9f
#define ANIMATION_LOOP_PERIOD (96.0f * PI)

#define JELLYFISH_COUNT 16
#define PHASE_STEP 13.0f

typedef struct {
    float curve_x;
    float curve_y;
    float radius_base;
} CurveSample;

CurveSample g_curve_samples[CURVE_SAMPLE_COUNT + 1];
Vector2 g_screen_points[CURVE_SAMPLE_COUNT + 1];
float g_radius_base_mean;

// 0 = free swimming
// 1 = centered
float g_follow_strength = 0.0f;

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

int updateScreenPoints(float animation_time, float phase_offset, int stride, float view_scale,
                       Vector2 view_origin) {
    const float drift_time = animation_time / 48.0f;
    const float bell_wave = sinf(animation_time * 0.5f + phase_offset);
    const float pulse_offset = 1.5f - bell_wave * bell_wave * bell_wave / 3.0f;
    const float anchor_phase =
        (g_radius_base_mean + pulse_offset) / 16.0f - drift_time + phase_offset;
    const float anchor_x = 99.0f * sinf(anchor_phase) * g_follow_strength;
    const float anchor_y = 99.0f * sinf(anchor_phase * 4.0f) * g_follow_strength;

    int point_count = 0;
    for (int sample_index = 0; sample_index <= CURVE_SAMPLE_COUNT; sample_index += stride) {
        const CurveSample *sample = &g_curve_samples[sample_index];
        float pulse_radius = sample->radius_base + pulse_offset;
        float drift_phase = pulse_radius / 16.0f - drift_time + phase_offset;
        float breath =
            powf(pulse_radius, sinf(pulse_radius * pulse_radius - animation_time + phase_offset));

        g_screen_points[point_count].x =
            (99.0f * sinf(drift_phase) - anchor_x + sample->curve_x * breath) * view_scale +
            view_origin.x;
        g_screen_points[point_count].y =
            (99.0f * sinf(drift_phase * 4.0f) - anchor_y + sample->curve_y * breath) * view_scale +
            view_origin.y;
        point_count++;
    }
    return point_count;
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
    float follow_target = 1.0f;
    float animation_time = 0.0f;
    bool paused = false;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) {
            paused = !paused;
        }
        if (IsKeyPressed(KEY_R)) {
            animation_time = 0.0f;
        }
        if (IsKeyPressed(KEY_F)) {
            g_follow_strength +=
                (follow_target - g_follow_strength) * fminf(1.0f, GetFrameTime() * 2.0f);
            g_follow_strength = (g_follow_strength > 0.5f) ? 0.0f : 1.0f;
        }
        if (IsKeyPressed(KEY_S)) {
            TakeScreenshot("jellyfish.png");
        }

        if (!paused) {
            animation_time += (PI / 20.0f) * (GetFrameTime() * 30.0f);
            if (animation_time >= ANIMATION_LOOP_PERIOD) {
                animation_time -= ANIMATION_LOOP_PERIOD;
            }
        }

        ensureTrailBuffer(&trail_buffer);

        const float buffer_width = (float)trail_buffer.texture.height;
        const float buffer_height = (float)trail_buffer.texture.height;

        const float pixel_scale = buffer_height / (float)GetScreenHeight();

        const float world_half_extent =
            CENTERED_HALF_EXTENT + (1.0f - g_follow_strength) * DRIFT_AMPLITUDE;
        const float view_scale = buffer_height * 0.5f * VIEW_FILL_RATIO / world_half_extent;
        const float school_alpha = 1.0f - g_follow_strength;
        const Vector2 view_origin = {buffer_width * 0.5f, buffer_height * 0.5f};

        float halo = fmaxf(1.5f * pixel_scale, HALO_THICKNESS * view_scale);
        float core = fmaxf(0.7f * pixel_scale, CORE_THICKNESS * view_scale);

        BeginTextureMode(trail_buffer);
        DrawRectangle(0, 0, (int)buffer_width, (int)buffer_height, TRAIL_FADE_COLOR);
        BeginBlendMode(BLEND_ADDITIVE);
        for (int jellyfish = 0; jellyfish < JELLYFISH_COUNT; jellyfish++) {
            if (jellyfish > 0 && school_alpha <= 0.004f) {
                break;
            }
            float alpha = (jellyfish == 0) ? 1.0f : school_alpha;
            int point_count = updateScreenPoints(animation_time, jellyfish * PHASE_STEP,
                                                 (jellyfish == 0) ? 1 : 4, view_scale, view_origin);
            DrawSplineLinear(g_screen_points, point_count, halo,
                             ColorAlpha(GLOW_HALO_COLOR, alpha));
            DrawSplineLinear(g_screen_points, point_count, core,
                             ColorAlpha(GLOW_CORE_COLOR, alpha));
        }
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

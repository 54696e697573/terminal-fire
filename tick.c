#include "header.h"

#define INJECTION_RADIUS 4
#define INJECTION_FORCE 10.0f
#define INJECTION_HEAT 10.0f
#define MAX_VELOCITY 1000.0f
#define BOUYANCY 0.01f

#define ADVECTION_DISTANCE 4.0f

#define PROJECTION_ITERATIONS 128
#define OVERRELAXATION 1.5f



static inline size_t index_grid(int x, int y) {
    return x + y * width;
}
static inline float clampf(float value, float min, float max) {
    return value < min ? min : (value > max ? max : value);
}

static inline void apply_forces(void) {
    for (int x = -INJECTION_RADIUS; x <= INJECTION_RADIUS; x++) {
        grid[index_grid(width / 2 + x, height - 2)].y = -INJECTION_FORCE;
        grid[index_grid(width / 2 + x, height - 2)].heat = INJECTION_HEAT;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const size_t index = index_grid(x, y);
            grid[index].x = clampf(grid[index].x, -MAX_VELOCITY, MAX_VELOCITY);
            grid[index].y = clampf(grid[index].y, -MAX_VELOCITY, MAX_VELOCITY); 
        }
    }

    for (int y = 0; y < height - 1; y++) {
        for (int x = 0; x < width - 1; x++) {
            float force = grid[index_grid(x, y)].heat * BOUYANCY;
            grid[index_grid(x, y)].y -= force / 2;
            grid[index_grid(x, y + 1)].y -= force / 2;
        }
    }
}

static inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}
static inline float bilinear(float a, float b, float c, float d, float t1, float t2) {
    return lerpf(
        lerpf(a, b, t1),
        lerpf(c, d, t1),
        t2
    );
}
static inline void advect(void) {
    memset(buffer, 0, max * sizeof(struct vector));
    for (int y = 0; y < height - 1; y++) {
        for (int x = 0; x < width - 1; x++) {
            const float x_velocity = (grid[index_grid(x, y)].x + grid[index_grid(x + 1, y)].x) / 2;
            const float y_velocity = (grid[index_grid(x, y)].y + grid[index_grid(x, y + 1)].y) / 2;
            const float old_x = clampf(x + 0.5 - x_velocity * deltatime * ADVECTION_DISTANCE, 0.0f, width - 2);
            const float old_y = clampf(y + 0.5 - y_velocity * deltatime * ADVECTION_DISTANCE, 0.0f, height - 2);

            const float u_x = old_x;
            const float u_y = old_y - 0.5f;
            const float v_x = old_x - 0.5f;
            const float v_y = old_y;
            const int int_u_x = (int)u_x;
            const int int_u_y = (int)u_y;
            const int int_v_x = (int)v_x;
            const int int_v_y = (int)v_y;

            const float old_x_velocity = bilinear(
                grid[index_grid(int_u_x, int_u_y)].x,
                grid[index_grid(int_u_x + 1, int_u_y)].x,
                grid[index_grid(int_u_x, int_u_y + 1)].x,
                grid[index_grid(int_u_x + 1, int_u_y + 1)].x,
                u_x - int_u_x,
                u_y - int_u_y
            );
            const float old_y_velocity = bilinear(
                grid[index_grid(int_v_x, int_v_y)].y,
                grid[index_grid(int_v_x + 1, int_v_y)].y,
                grid[index_grid(int_v_x, int_v_y + 1)].y,
                grid[index_grid(int_v_x + 1, int_v_y + 1)].y,
                v_x - int_v_x,
                v_y - int_v_y
            );

            buffer[index_grid(x, y)].x     += old_x_velocity / 2;
            buffer[index_grid(x + 1, y)].x += old_x_velocity / 2;
            buffer[index_grid(x, y)].y     += old_y_velocity / 2;
            buffer[index_grid(x, y + 1)].y += old_y_velocity / 2;

            const float heat_x = old_x - 0.5f;
            const float heat_y = old_y - 0.5f;
            const int int_heat_x = (int)heat_x;
            const int int_heat_y = (int)heat_y;

            const float old_heat = bilinear(
                grid[index_grid(int_heat_x, int_heat_y)].heat,
                grid[index_grid(int_heat_x + 1, int_heat_y)].heat,
                grid[index_grid(int_heat_x, int_heat_y + 1)].heat,
                grid[index_grid(int_heat_x + 1, int_heat_y + 1)].heat,
                heat_x - int_heat_x,
                heat_y - int_heat_y
            );
            buffer[index_grid(x, y)].heat = old_heat;
        }
    }
    memcpy(grid, buffer, max * sizeof(struct vector));
}

static inline void project(void) {
    for (int iteration = 0; iteration < PROJECTION_ITERATIONS; iteration++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                struct vector * right = NULL;
                struct vector * down  = NULL;

                const bool up_edge    = y == 0;
                const bool right_edge = x == width - 1;
                const bool down_edge  = y == height - 1;
                const bool left_edge  = x == 0;

                struct vector * this = grid + index_grid(x, y);
                if (!right_edge) right = grid + index_grid(x + 1, y);
                if (!down_edge) down  = grid + index_grid(x, y + 1);

                const int edge_count = 4 - (up_edge + right_edge + down_edge + left_edge);

                float divergence = 0;

                if (!right_edge) divergence += right->x;
                if (!down_edge)  divergence += down->y;
                if (!left_edge)  divergence -= this->x;
                if (!up_edge)    divergence -= this->y;

                float force = divergence / edge_count * OVERRELAXATION;

                if (!right_edge) right->x -= force;
                if (!down_edge)  down->y  -= force;
                if (!left_edge)  this->x  += force;
                if (!up_edge)    this->y  += force;
            }
        }
    }
}

static inline void get_character(float x, float y, char *string) {
    if (abs(x) + abs(y) < 2 && mode != BLOCKS) {
        string[0] = 0xE2;
        string[1] = 0x81;
        string[2] = 0x96;
        return;
    }
    float abs_x = fabsf(x);
    float abs_y = fabsf(y);
    if (mode == ARROWS) {
        if (abs_x > 2 * abs_y) {
            if (x > 0) {
                string[0] = 0xE2;
                string[1] = 0x86;
                string[2] = 0x92;
                return;
            }
            string[0] = 0xE2;
            string[1] = 0x86;
            string[2] = 0x90;
            return;
        }
        if (abs_y > 2 * abs_x) {
            if (y > 0) {
                string[0] = 0xE2;
                string[1] = 0x86;
                string[2] = 0x93;
                return;
            }
            string[0] = 0xE2;
            string[1] = 0x86;
            string[2] = 0x91;
            return;
        } else if ((x > 0) == (y > 0)) {
            if (x > 0) {
                string[0] = 0xE2;
                string[1] = 0x86;
                string[2] = 0x98;
                return;
            }
            string[0] = 0xE2;
            string[1] = 0x86;
            string[2] = 0x96;
            return;
        }
        if (x > 0) {
            string[0] = 0xE2;
            string[1] = 0x86;
            string[2] = 0x97;
            return;
        }
        string[0] = 0xE2;
        string[1] = 0x86;
        string[2] = 0x99;
        return;
    } else if (mode == STREAMLINES) {
        if (abs_x > 2 * abs_y) {
            string[0] = 0xE2;
            string[1] = 0x94;
            string[2] = 0x80;
            return;
        }
        if (abs_y > 2 * abs_x) {
            string[0] = 0xE2;
            string[1] = 0x94;
            string[2] = 0x82;
            return;
        } else if ((x > 0) == (y > 0)) {
            string[0] = 0xE2;
            string[1] = 0x95;
            string[2] = 0xB2;
            return;
        }
        string[0] = 0xE2;
        string[1] = 0x95;
        string[2] = 0xB1;
        return;
    }
    string[0] = 0xE2;
    string[1] = 0x96;
    string[2] = 0x88;
}
static inline int get_color(float heat) {
    float color = 232 + heat * 2;
    return (int)clampf(color, 232.0f, 248.0f);
}
static inline void draw(void) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const size_t index = index_grid(x, y);
            const float this_x = grid[index].x;
            const float this_y = grid[index].y;

            char character[3];
            get_character(this_x, this_y, character);
            const char horizontal[3] = { 0xE2, 0x94, 0x80 };
            const char vertical[3] = { 0xE2, 0x94, 0x82 };
            const bool draw = mode != STREAMLINES ?
                true :
                memcmp(character, horizontal, 3) == 0 ?
                    true :
                    memcmp(character, vertical, 3) == 0 ?
                        x % 2 == 0 :
                        (x + y) % 2 == 0;
            const int color = draw ? get_color(grid[index].heat) : 232;

            char string[14];
            memcpy(string, "\033[38;5;___m", 11);
            string[7] = '0' + (color / 100);
            string[8] = '0' + ((color % 100) / 10);
            string[9] = '0' + (color % 10);
            memcpy(string + 11, character, 3);

            memcpy(frame + 14 * index, string, 14);
        }
    }

    write(STDOUT_FILENO, "\033[H\033[48;5;232m", 14);
    write(STDOUT_FILENO, frame, max * 14 - 14);
    write(STDOUT_FILENO, "\033[0m", 4);
}

void tick(void) {
    apply_forces();
    advect();
    project();

    draw();
}


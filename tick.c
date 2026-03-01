#include "header.h"

#define INJECTION_RADIUS 2
#define INJECTION_FORCE 100.0f
#define MAX_VELOCITY 500.0f

#define ADVECTION_DISTANCE 1.0f

#define PROJECTION_ITERATIONS 128
#define OVERRELAXATION 1.5f



static inline size_t index_grid(int x, int y) {
    return x + y * width;
}
static inline float clampf(float value, float min, float max) {
    return value < min ? min : (value > max ? max : value);
}

static inline void apply_forces(void) {
    for (int y = -INJECTION_RADIUS; y <= INJECTION_RADIUS; y++) {
        grid[index_grid(1, height / 2 + y)].x = INJECTION_FORCE;
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const size_t index = index_grid(x, y);
            grid[index].x = clampf(grid[index].x, -MAX_VELOCITY, MAX_VELOCITY);
            grid[index].y = clampf(grid[index].y, -MAX_VELOCITY, MAX_VELOCITY); 
        }
    }
}

static inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}
static inline float bilinearf(float a, float b, float c, float d, float t1, float t2) {
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
            const float old_x = clampf(x + 0.5 - x_velocity * deltatime * ADVECTION_DISTANCE, 0.5f, width - 3);
            const float old_y = clampf(y + 0.5 - y_velocity * deltatime * ADVECTION_DISTANCE, 0.5f, height - 3);

            const float u_x = old_x;
            const float u_y = old_y - 0.5f;
            const float v_x = old_x - 0.5f;
            const float v_y = old_y;
            const int int_u_x = (int)u_x;
            const int int_u_y = (int)u_y;
            const int int_v_x = (int)v_x;
            const int int_v_y = (int)v_y;

            const float old_x_velocity = bilinearf(
                grid[index_grid(int_u_x, int_u_y)].x,
                grid[index_grid(int_u_x + 1, int_u_y)].x,
                grid[index_grid(int_u_x, int_u_y + 1)].x,
                grid[index_grid(int_u_x + 1, int_u_y + 1)].x,
                u_x - int_u_x,
                u_y - int_u_y
            );
            const float old_y_velocity = bilinearf(
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
    if (x == 0 && y == 0) {
        string[0] = 0xE3;
        string[1] = 0x80;
        string[2] = 0x80;
        return;
    }
    float abs_x = fabsf(x);
    float abs_y = fabsf(y);
    if (ARROWS) {
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
    }

    if (abs_x > 2 * abs_y) {
        string[0] = 0x5F;
        string[1] = 0xCC;
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
static inline int get_color(float velocity) {
    int color = 232 + (int)(velocity / 4);
    return color > 248 ? 248 : color;
}
static inline void draw(void) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const size_t index = index_grid(x, y);
            const float this_x = grid[index].x;
            const float this_y = grid[index].y;

            char character[3];
            get_character(this_x, this_y, character);
            const int color = get_color(abs(grid[index].x) + abs(grid[index].y));

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


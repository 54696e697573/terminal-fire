#include "header.h"

#define PROJECTION_ITERATIONS 32
#define OVERRELAXATION 1.75



static inline size_t index_grid(int x, int y) {
    return x + y * width;
}

static inline void apply_forces(void) {
    grid[max / 2 + width / 2].y = -1000;
}

static inline void project(void) {
    for (int iteration = 0; iteration < PROJECTION_ITERATIONS; iteration++) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                struct vector * const this  = grid + index_grid(x, y);
                struct vector * const right = grid + index_grid(x + 1, y);
                struct vector * const down  = grid + index_grid(x, y + 1);

                const bool up_edge    = y == 0;
                const bool right_edge = x == width - 1;
                const bool down_edge  = y == height - 1;
                const bool left_edge  = x == 0;

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

static inline void advect(void) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            struct vector * const this  = grid + index_grid(x, y);
            struct vector * const right = grid + index_grid(x + 1, y);
            struct vector * const down  = grid + index_grid(x, y + 1);

            float x_velocity = (this->x - right->x) / 2;
            float y_velocity = (this->y - down->y) / 2;
            float x_offset = x + 0.5 + x_velocity * deltatime;
            float y_offset = y + 0.5 + y_velocity * deltatime;
            int x_floored = floorf(x_offset);
            int y_floored = floorf(y_offset);
            float in_x = x_offset - x_floored;
            float in_y = y_offset - y_floored;

            struct vector * const new_this  = grid + index_grid(x_floored, y_floored);
            struct vector * const new_right = grid + index_grid(x_floored + 1, y_floored);
            struct vector * const new_down  = grid + index_grid(x, y + 1);

            
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
    int color = 232 + (int)(velocity / 100 * 24);
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
            const int color = get_color(
                fabsf(this_x) +
                fabsf(this_y) +
                fabsf(grid[index_grid(x + 1, y)].x) +
                fabsf(grid[index_grid(x, y + 1)].y)
            );

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
    project();
    //advect();

    draw();
}


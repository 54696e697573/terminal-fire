#include "header.h"

#define PROJECT_ITERATIONS 4
#define ADVECTION_LENGTH 0.01



static inline void apply_forces(void) {
    grid[(width >> 1) + (max >> 1)].x = 0;
    grid[(width >> 1) + (max >> 1)].y = 0;
}

static inline void project(void) {
    for (size_t index = 0; index < max; index++) {
        struct vector * const this = grid + index;
        struct vector * const down = grid + ((index + width) % max);
        struct vector * const right = grid + ((index + 1) % max);

        long long divergence = (
            this->x +
            this->y -
            right->x -
            down->y
        );
        int force = (int)(divergence / 2.5);

        this->x -= force;
        this->y -= force;
        right->x += force;
        down->y += force;
    }
}

static inline void advect(void) {
    for (int index = 0; index < max; index++) {
        struct vector *this = grid + index;
        size_t new_index = (
            index -
            (int)(ADVECTION_LENGTH * deltatime * this->x) - 
            (int)( ADVECTION_LENGTH * deltatime * this->y * width)
        ) % max;

        struct vector *that = grid + new_index;
        buffer[index] = *that;
    }
    memcpy(grid, buffer, max * sizeof(struct vector));
}

static inline char get_character(int x, int y) {
    if (x == 0 && y == 0) return ' ';
    int abs_x = abs(x);
    int abs_y = abs(y);
    if (abs_x > 2 * abs_y) return '=';
    if (abs_y > 2 * abs_x) return '|';
    if ((x > 0) == (y > 0)) return '/';
    return '\\';
}
static inline int get_color(unsigned int heat) {
    if (heat < 100) {
        return 232;
    } else if (heat < 200) {
        return 52;
    } else if (heat < 300) {
        return 160;
    } else if (heat < 400) {
        return 196;
    } else if (heat < 500) {
        return 202;
    } else if (heat < 600) {
        return 208;
    } else if (heat < 700) {
        return 214;
    } else if (heat < 800) {
        return 220;
    } else if (heat < 900) {
        return 226;
    }
    return 228;

}
static inline void draw(void) {
    for (size_t index = 0; index < max; index++) {
        int x = grid[index].x;
        int y = grid[index].y;

        char character = get_character(x, y);
        int color = get_color(grid[index].heat);

        char string[12];
        memcpy(string, "\033[38;5;???m", 11);
        string[7] = '0' + (color / 100);
        string[8] = '0' + ((color % 100) / 10);
        string[9] = '0' + (color % 10);
        string[11] = character;

        memcpy(frame + 12 * index, string, 12);
    }
    
    write(STDOUT_FILENO, "\033[H\033[48;5;232m", 14);
    write(STDOUT_FILENO, frame, max * 12);
    write(STDOUT_FILENO, "\033[0m", 4);
}

void tick(void) {
    apply_forces();
    for (int iter = 0; iter < PROJECT_ITERATIONS; iter++) project();
    advect();
    draw();
}

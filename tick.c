#include "header.h"

#define PROJECT_ITERATIONS 64
#define ADVECTION_LENGTH 0.0001
#define HEAT_DISPERSION 0.25
#define DISPERSE_ITERATIONS 4
#define BOUYANCY 1



static inline void apply_forces(void) {
    for (int index = -5; index <= 5; index++) {
        grid[max - (width >> 1) + index].heat = 10000;
    }

    for (int index = 0; index < max; index++) {
        grid[index].y -= (int)(grid[index].heat * deltatime * BOUYANCY);
    }

    for (int index = 0; index < width; index++) {
        grid[index] = (struct vector){ 0, 0, 0, };
    }
    for (int index = 0; index < max; index += width) {
        grid[index] = (struct vector){ 0, 0, 0, };
    }
    for (int index = width - 1; index < max; index += width) {
        grid[index] = (struct vector){ 0, 0, 0, };
    }
}

static inline void project(void) {
    for (size_t index = 0; index < max; index++) {
        struct vector * const this = grid + index;
        struct vector * const down = buffer + (index + width > max ? index : index + width);
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
            index +
            (int)(ADVECTION_LENGTH * deltatime * this->x) +
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
    if (abs_x > 2 * abs_y) return x > 0 ? '>' : '<';
    if (abs_y > 2 * abs_x) return y > 0 ? 'v' : '^';
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

static inline void disperse(void) {
    memcpy(buffer, grid, max * sizeof(struct vector));
    for (int index = 0; index < max; index++) {
        struct vector * const this = grid + index;
        struct vector * const up = buffer + ((index - width + max) % max);
        struct vector * const right = buffer + ((index + 1) % max);
        struct vector * const down = buffer + (index + width > max ? index : index + width);
        struct vector * const left = buffer + ((index - 1 + max) % max);
        
        const int heat_out = (int)(this->heat * HEAT_DISPERSION);
        buffer[index].heat = this->heat - heat_out;
        up->heat += heat_out / 4;
        right->heat += heat_out / 4;
        down->heat += heat_out / 4;
        left->heat += heat_out / 4;
    }
    memcpy (grid, buffer, max * sizeof(struct vector));
}

void tick(void) {
    for (int iter = 0; iter < PROJECT_ITERATIONS; iter++) {
        project();
        apply_forces();
    }
    for (int iter = 0; iter < DISPERSE_ITERATIONS; iter++) disperse();
    advect();

    draw();
}

#include "header.h"

#define PROJECT_ITERATIONS 4



static inline void apply_forces(void) {
    // currently not needed
}

static inline void project(void) {
    for (size_t index = 0; index < max; index++) {
        struct vector *this = grid + index;
        struct vector *down = grid + ((index + width) % max);
        struct vector *right = grid + ((index + 1) % max);

        short divergence = (
            this->x +
            this->y -
            right->x -
            down->y
        );
        char force = divergence >> 2;

        this->x -= force;
        this->y -= force;
        right->x += force;
        down->y += force;
    }
}

static inline void advect(void) {
    // WIP
}

const char VECTOR_CHARACTERS[] = {
    '\\', '|', '/',
    '-', ' ', '/',
    '/', '|', '\\'
};
static inline char sign(char x) {
    return (x > 0) - (x < 0);
}
static inline void draw(void) {
    for (size_t index = 0; index < max; index++) {
        char character = VECTOR_CHARACTERS[sign(grid[index].x) + sign(grid[index].y) * 3 + 4];
        buffer[index] = character;
    }

    write(STDOUT_FILENO, buffer, max);
}

void tick(void) {
    //apply_forces();
    for (int iter = 0; iter < PROJECT_ITERATIONS; iter++) project();
    advect();
    draw();
}

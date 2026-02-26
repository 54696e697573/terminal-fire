#include "header.h"

#define PROJECTION_ITERATIONS 32
#define OVERRELAXATION 1.9



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
    // WIP
}

static inline char get_character(float x, float y) {
    if (x == 0 && y == 0) return ' ';
    float abs_x = fabsf(x);
    float abs_y = fabsf(y);
    if (abs_x > 2 * abs_y) return ARROWS ? (x > 0 ? '>' : '<') : '=';
    if (abs_y > 2 * abs_x) return ARROWS ? (y > 0 ? 'v' : '^') : '|';
    if ((x > 0) == (y > 0)) return '\\';
    return '/';
}
static inline int get_color(float velocity) {
    int color = 232 + (int)(velocity / 100 * 24);
    return color > 248 ? 248 : color;
}
static inline void draw(void) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width ; x++) {
            const size_t index = index_grid(x, y);
            const float this_x = grid[index].x;
            const float this_y = grid[index].y;

            const char character = get_character(this_x, this_y);
            const int color = get_color(
                fabsf(this_x) +
                fabsf(this_y) +
                fabsf(grid[index_grid(x + 1, y)].x) +
                fabsf(grid[index_grid(x, y + 1)].y)
            );

            char string[12];
            memcpy(string, "\033[38;5;???m", 11);
            string[7] = '0' + (color / 100);
            string[8] = '0' + ((color % 100) / 10);
            string[9] = '0' + (color % 10);
            string[11] = character;

            memcpy(frame + 12 * index, string, 12);
        }
    }
    
    write(STDOUT_FILENO, "\033[H\033[48;5;232m", 14);
    write(STDOUT_FILENO, frame, max * 12);
    write(STDOUT_FILENO, "\033[0m", 4);
}

void tick(void) {
    apply_forces();
    project();
    advect();

    draw();
}


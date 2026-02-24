#include "header.h"

#define NANOSECONDS 1000000000L



volatile bool resize_signal = true;
volatile bool exit_signal = false;

size_t width;
size_t height;
size_t max;
struct vector *grid = NULL;
char *buffer = NULL;
static void realloc_assert(void *temp) {
    if (!temp) {
        fprintf(stderr, "Error while reallocating grid due to resizing.");
        exit(EXIT_FAILURE);
    }
}
static void resize(void) {
    resize_signal = false;

    struct winsize ws;
    assert(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0);
    width = ws.ws_col;
    height = ws.ws_row;
    size_t past_max = max;
    max = width * height;
    
    void *temp;
    temp = realloc(grid, sizeof(struct vector[max]));
    realloc_assert(temp);
    grid = temp;

    temp = realloc(buffer, sizeof(char[max]));
    realloc_assert(temp);
    buffer = temp;

    if (max > past_max) {
        memset(grid + past_max, 0, max - past_max);
    }
}
static void set_resize_signal(int signal) {
    (void)signal;
    resize_signal = true;
}

static inline double get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / NANOSECONDS;
}
static inline void sleep_time(double time) {
    struct timespec ts;
    ts.tv_sec = (time_t)time;
    ts.tv_nsec = (long)((time - ts.tv_sec) * NANOSECONDS);
    nanosleep(&ts, NULL);
}

static void exit_program(int signal) {
    (void)signal;
    exit_signal = true;
}

double deltatime = 0.0;
int main() {
    assert(signal(SIGWINCH, set_resize_signal) == 0);
    assert(signal(SIGINT, exit_program) == 0);

    resize();

    const double target_fps = 60.0;
    double target_time = 1.0 / target_fps;

    while (!exit_signal) {
        double start = get_time();
        if (resize_signal) resize();
        tick();
        double end = get_time();
        
        double frame_time = end - start;
        double needed = target_time - frame_time;
        if (needed > 0) sleep_time(needed);

        deltatime = frame_time > target_time ? frame_time : target_time;
    }

    free(grid);
    free(buffer);

    return 0;
}


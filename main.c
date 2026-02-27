#include "header.h"

#define NANOSECONDS 1000000000L



bool ARROWS = false;

volatile bool resize_signal = true;
volatile bool exit_signal = false;

size_t width;
size_t height;
size_t max;
struct vector *grid = NULL;
struct vector *buffer = NULL;
char *frame = NULL;
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
    temp = realloc(grid, max * sizeof(struct vector));
    realloc_assert(temp);
    grid = temp;

    temp = realloc(buffer, max * sizeof(struct vector));
    realloc_assert(temp);
    buffer = temp;

    temp = realloc(frame, max * 14 * sizeof(char));
    realloc_assert(temp);
    frame = temp;

    if (max > past_max) {
        memset(grid + past_max, 0, (max - past_max) * sizeof(struct vector));
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
int main(int argc, char *argv[]) {
    for (int index = 1; index < argc; index++) {
        if (strcmp(argv[index], "-a") == 0) ARROWS = true;
    }

    assert(signal(SIGWINCH, set_resize_signal) == 0);
    assert(signal(SIGINT, exit_program) == 0);

    resize();

    const double target_fps = 60.0;
    double target_time = 1.0 / target_fps;

    struct termios original;
    tcgetattr(STDIN_FILENO, &original);
    struct termios new = original;
    new.c_lflag &= ~ECHO;
    new.c_lflag &= ~ICANON;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new);

    write(STDOUT_FILENO, "\033[?1049h\033[?25l", 14);

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

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);

    write(STDOUT_FILENO, "\033[?1049l\033[?25h", 14);

    free(grid);
    free(buffer);
    free(frame);

    return 0;
}


#include "header.h"



static inline char upper_mask(char n) {
    return n & 0b11110000;
}
static inline char upper(char n) {
    return upper_mask(n) >> 4;
}
static inline char lower(char n) {
    return n & 0b00001111;
}

static inline void apply_forces(void) {
    //WIP
}
static inline void project(void) {
    //WIP
}
static inline void advect(void) {
    //WIP
}

void tick(void) {
    apply_forces();
    project();
    advect();
}

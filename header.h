#ifndef HEADER_H
#define HEADER_H

#include <assert.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>



void tick(void);

extern double deltatime;

extern size_t width;
extern size_t height;
extern size_t max;
extern char *grid;
extern char *buffer;
extern char *frame;

#endif

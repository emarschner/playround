#ifndef __COMMON_H_
#define __COMMON_H_

#define MY_PI 3.1415926
#define DEFAULT_PORT 10101

// RtAudio Settings
#define SAMPLE float
#define MY_SRATE 44100
#define NUM_CHANNS 1
#define BUFFER_SIZE 512

// Target 30 FPS
#define TIMER_MSECS 33

// Control rate of pluckers
#define ANGLE_DELTA 0.05
#define PLUCKER_SPEED 10

#include <uuid/uuid.h>

#endif

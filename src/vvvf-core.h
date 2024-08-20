#include "vvvf-struct.h"

#ifndef VVVF_CALCULATE_H
#define VVVF_CALCULATE_H

typedef struct {
    VvvfValues *status;
    PwmCalculateValues *pwm;
    double initialPhase;
} CalculateParam;

char vvvfTwoLevel(CalculateParam *);
char vvvfThreeLevel(CalculateParam *);

#endif
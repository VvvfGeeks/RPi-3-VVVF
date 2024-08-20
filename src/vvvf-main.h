#ifndef VVVF_MAIN_H
#define VVVF_MAIN_H

#include "vvvf-struct.h"
#include "vvvf-core.h"

#define ENABLE_DISPLAY

void calculatePhases(
    PhaseStatus *outU,              // OUTPUT U
    PhaseStatus *outV,              // OUTPUT V
    PhaseStatus *outW,              // OUTPUT W
    VvvfValues *status,             // SINE AND SAW ANGLE FREQ AND TIME
    VvvfSoundFunction soundFunction // CALCULATE SOUND FUNCTION
);

// VVVF GPIO
void requestStatusVvvfGpio();
bool canGetStatusVvvfGpio();
VvvfValues getStatusVvvfGpio(void);
void setStatusVvvfGpio(VvvfValues *want);
VvvfSoundFunction getSoundVvvfGpio();
void setSoundVvvfGpio(VvvfSoundFunction _sound);

extern uint64_t calcTimeElapse;
void taskCalculationPhases(void *param);
#endif
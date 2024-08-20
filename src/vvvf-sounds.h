#ifndef VVVF_WAVE_H
#define VVVF_WAVE_H

#include "vvvf-struct.h"

typedef void (* VvvfSoundFunction)(VvvfValues *, PwmCalculateValues *);
extern VvvfSoundFunction vvvf_sounds[];
extern const int vvvf_sounds_len;

#endif

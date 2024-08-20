#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "vvvf-struct.h"
#include "vvvf-sounds.h"
#include "vvvf-hardware.h"
#include "vvvf-core.h"
#include "vvvf-main.h"
#include "pi-values.h"

#include "task.h"

void calculatePhases(
    PhaseStatus *outU,              // OUTPUT U
    PhaseStatus *outV,              // OUTPUT V
    PhaseStatus *outW,              // OUTPUT W
    VvvfValues *status,             // All values
    VvvfSoundFunction soundFunction // CALCULATE SOUND FUNCTION
)
{
    PwmCalculateValues pwm;
    soundFunction(status, &pwm);
    PhaseStatus (*phaseCalculate)(CalculateParam *) = pwm.level == 2 ? vvvfTwoLevel : vvvfThreeLevel;

    CalculateParam param;
    param.pwm = &pwm;
    param.status = status;
    
    // Phase U
    param.initialPhase = M_PI_6;
    if(outU != 0) *outU = phaseCalculate(&param);

    // Phase V
    param.initialPhase = M_2PI_3 + M_PI_6;
    if(outV != 0) *outV = phaseCalculate(&param);

    // Phase W
    param.initialPhase = M_4PI_3 + M_PI_6;
    if(outW != 0) *outW = phaseCalculate(&param);
}

// VARIABLES FOR MAIN ROUTINE ON GPIO
static bool updateStatusGpio = false, wantStatusGpio = false, getStatusGpio = false;
static VvvfValues statusGpio = {
    false, false, 0, false, 0,
    true, true,
    0, 0, 0, 0, 0, 0, 0,
}, _statusGpio;
static bool updateSoundGpio = false;
static VvvfSoundFunction soundGpio, _soundGpio;

void requestStatusVvvfGpio(){
    getStatusGpio = false;
    wantStatusGpio = true;
}

bool canGetStatusVvvfGpio(){
    return getStatusGpio;
}

VvvfValues getStatusVvvfGpio(){
    return _statusGpio;
}

void setStatusVvvfGpio(VvvfValues *want){
    _statusGpio = *want;
    updateStatusGpio = true;
}

VvvfSoundFunction getSoundVvvfGpio(){
    return soundGpio;
}
void setSoundVvvfGpio(VvvfSoundFunction _sound){
    _soundGpio = _sound;
    updateSoundGpio = true;
}

uint64_t calcTimeElapse = 0;

#ifdef ENABLE_DISPLAY
void processScreenBufferRender();
#endif

void taskCalculationPhases(void *param)
{
    uint64_t _s = 0;
    PhaseStatus outputU = 0, outputV = 0, outputW = 0;
    PhaseStatus preOutputU = 0, preOutputV = 0, preOutputW = 0;
    while (true)
    {
        _s = timer_getTickCount64();
        
        if(updateSoundGpio) { updateSoundGpio = false; soundGpio = _soundGpio; }
        if(updateStatusGpio) { updateStatusGpio = false; statusGpio = _statusGpio; }

        double _t = calcTimeElapse / 1000000.0;
        statusGpio.saw_time += _t;
        statusGpio.sin_time += _t;
        statusGpio.generation_current_time = timer_getTickCount64() / 1000000.0;
        calculatePhases(&outputU, &outputV, &outputW, &statusGpio, soundGpio);

        if(abs(outputU - preOutputU) == 2) { preOutputU = outputU; outputU = 1; }
        if(abs(outputV - preOutputV) == 2) { preOutputV = outputV; outputV = 1; }
        if(abs(outputW - preOutputW) == 2) { preOutputW = outputW; outputW = 1; }

        if(!statusGpio.mascon_off){
            if(statusGpio.free_run){
                double freq_change = statusGpio.free_freq_change * _t;
                double final_freq = statusGpio.wave_stat + freq_change;
                if (statusGpio.sin_angle_freq * M_1_2PI <= final_freq)
                {
                    statusGpio.wave_stat = statusGpio.sin_angle_freq * M_1_2PI;
                    statusGpio.free_run = false;
                }
                else
                {
                    statusGpio.wave_stat = final_freq;
                    statusGpio.free_run = true;
                }
            }
        }
        else
        {
            double freq_change = statusGpio.free_freq_change * _t;
            double final_freq = statusGpio.wave_stat - freq_change;
            statusGpio.wave_stat = final_freq > 0 ? final_freq : 0;
            statusGpio.free_run = true;
        }

        if(wantStatusGpio) { wantStatusGpio = false; _statusGpio = statusGpio; getStatusGpio = true; }

#ifdef ENABLE_DISPLAY
        while(timer_getTickCount64() - _s < 4){
            processScreenBufferRender();
        }
#endif
        while(timer_getTickCount64() - _s < 5);
        setPhasePinStatus(outputU, outputV, outputW);
        calcTimeElapse = timer_getTickCount64() - _s;
    }
}
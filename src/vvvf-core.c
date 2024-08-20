#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#include "pi-values.h"
#include "vvvf-core.h"
#include "vvvf-struct.h"
#include "vvvf-switch-angle.h"

static double getSaw(double x)
{
	double result;
	double fixed_x = x - (double)((int)(x * M_1_2PI) * M_2PI);
	if (0 <= fixed_x && fixed_x < M_PI_2)
		result = M_2_PI * fixed_x;
	else if (M_PI_2 <= fixed_x && fixed_x < 3.0 * M_PI_2)
		result = -M_2_PI * fixed_x + 2;
	else
		result = M_2_PI * fixed_x - 4;
	return -result;
}

static double getSin(double x, double amplitude)
{
	return sin(x) * amplitude;
}

static char modulateSin(double sin_value, double saw_value)
{
	return sin_value > saw_value;
}

static char get3LevelP1(double x, double voltage)
{
	double sine = getSin(x, 1);
	signed char D = sine > 0 ? 1 : -1;
	double voltage_fix = D * (1 - voltage);
	signed char gate = (D * (sine - voltage_fix) > 0) ? D : 0;
	gate += 1;
	return gate;
}

static char getWideP3(double x, double voltage, bool saw_oppose)
{
	double sin = getSin(x, 1);
	double saw = getSaw(x);
	if (saw_oppose)
		saw = -saw;
	double pwm = ((sin - saw > 0) ? 1 : -1) * voltage;
	double nega_saw = (saw > 0) ? saw - 1 : saw + 1;
	char gate = modulateSin(pwm, nega_saw) << 1;
	return gate;
}

static char getPulseWithSaw(double x, double voltage, double carrier_mul, bool saw_oppose)
{
	double carrier_saw = -getSaw(carrier_mul * x);
	double saw = -getSaw(x);
	if (saw_oppose)
		saw = -saw;
	double pwm = (saw > 0) ? voltage : -voltage;
	char gate = modulateSin(pwm, carrier_saw) << 1;
	return gate;
}

static char getPulseWithSwitchAngle(
	double alpha1,
	double alpha2,
	double alpha3,
	double alpha4,
	double alpha5,
	double alpha6,
	double alpha7,
	int flag,
	double x)
{
	double theta = x - (double)((int)(x * M_1_2PI) * M_2PI);

	int PWM_OUT = (((((theta <= alpha2) && (theta >= alpha1)) || ((theta <= alpha4) && (theta >= alpha3)) || ((theta <= alpha6) && (theta >= alpha5)) || ((theta <= M_PI - alpha1) && (theta >= M_PI - alpha2)) || ((theta <= M_PI - alpha3) && (theta >= M_PI - alpha4)) || ((theta <= M_PI - alpha5) && (theta >= M_PI - alpha6))) && ((theta <= M_PI) && (theta >= 0))) || (((theta <= M_PI - alpha7) && (theta >= alpha7)) && ((theta <= M_PI) && (theta >= 0)))) || ((!(((theta <= alpha2 + M_PI) && (theta >= alpha1 + M_PI)) || ((theta <= alpha4 + M_PI) && (theta >= alpha3 + M_PI)) || ((theta <= alpha6 + M_PI) && (theta >= alpha5 + M_PI)) || ((theta <= M_2PI - alpha1) && (theta >= M_2PI - alpha2)) || ((theta <= M_2PI - alpha3) && (theta >= M_2PI - alpha4)) || ((theta <= M_2PI - alpha5) && (theta >= M_2PI - alpha6))) && ((theta <= M_2PI) && (theta >= M_PI))) && !((theta <= M_2PI - alpha7) && (theta >= M_PI + alpha7)) && (theta <= M_2PI) && (theta >= M_PI)) ? 1 : -1;

	int gate = flag == 'A' ? -PWM_OUT + 1 : PWM_OUT + 1;
	return (char)gate;
}

static int getPulseNum(PulseModeNames mode)
{
	if (mode == Async)
		return -1;
	if (mode == P_1)
		return 1;
	if (mode == P_Wide_3)
		return 0;
	if (mode == P_5)
		return 6;
	if (mode == P_7)
		return 9;
	if (mode == P_10)
		return 10;
	if (mode == P_11)
		return 15;
	if (mode == P_12)
		return 12;
	if (mode == P_18)
		return 18;

	if ((int)mode <= (int)P_61)
		return 3 + (2 * ((int)mode - (int)P_3));

	return getPulseNum((PulseModeNames)((int)mode - (int)P_61));
}

// Random carrier frequency -range + base ~ range + base
static int getRandomFrequency(VvvfValues *status, PwmCalculateValues *pwm)
{
	if (
		status->pre_saw_random_freq == 0 || 
		status->random_freq_pre_time + pwm->carrier_freq.interval <= status->generation_current_time
	)
	{
		int random_v = rand();
		int diff_freq = random_v % (int)pwm->carrier_freq.range;
		if (random_v & 0x01) diff_freq = -diff_freq;
		
		status->pre_saw_random_freq = pwm->carrier_freq.base_freq + diff_freq;	//Sets new status
		status->random_freq_pre_time = status->generation_current_time;
	}

	return status->pre_saw_random_freq;
}

static int solveCarrierFrequency(VvvfValues *status, PwmCalculateValues *pwm){
	CarrierFreq carrier = pwm->carrier_freq;
	if(!status->allow_random_freq_move) return carrier.base_freq;
	if((int)(carrier.range)!=0) return getRandomFrequency(status, pwm);

	return carrier.base_freq;
}

char vvvfThreeLevel(CalculateParam *param)
{
	VvvfValues *status = param->status;
	PwmCalculateValues *pwm = param->pwm;
	double initialPhase = param->initialPhase;

	// Set to minimum frequency
	if(status->sin_angle_freq < pwm->min_freq * M_2PI && status->wave_stat > 0){
		status->v_sin_angle_freq = pwm->min_freq * M_2PI;
		status->allow_sine_time_change = false;
	}
	else{
		status->v_sin_angle_freq = status->sin_angle_freq;
		status->allow_sine_time_change = true;
	}

	if (pwm->none) return 0;

	if (pwm->pulse_mode.pulse_name == Async)
	{
		double carrier_angle_freq = solveCarrierFrequency(status, pwm) * M_2PI;
		status->saw_time *= status->saw_angle_freq / carrier_angle_freq;
		status->saw_angle_freq = carrier_angle_freq;

		double sin_value = getSin(status->sin_time * status->v_sin_angle_freq + initialPhase, pwm->amplitude);
		double saw_value = getSaw(status->saw_time * status->saw_angle_freq);

		double changed_saw = (pwm->dipolar != -1 ? pwm->dipolar : 0.5) * saw_value;
		char pwm_value = modulateSin(sin_value, changed_saw + 0.5) + modulateSin(sin_value, changed_saw - 0.5);

		return pwm_value;
	}
	else
	{
		status->saw_time = status->sin_time;
		status->saw_angle_freq = status->v_sin_angle_freq;

		double _t = status->sin_time * status->v_sin_angle_freq + initialPhase;

		if (pwm->pulse_mode.pulse_name == P_1)
		{
			if(pwm->pulse_mode.alt_mode == Alt1)
				return get3LevelP1(_t, pwm->amplitude);
		}

		double sin_value = getSin(_t , pwm->amplitude);
		double saw_value = getSaw(getPulseNum(pwm->pulse_mode.pulse_name) * _t);

		if ((int)pwm->pulse_mode.pulse_name > (int)P_61)
			saw_value = -saw_value;

		double changed_saw = (pwm->dipolar != -1 ? pwm->dipolar : 0.5) * saw_value;
		char pwm_value = modulateSin(sin_value, changed_saw + 0.5) + modulateSin(sin_value, changed_saw - 0.5);

		return pwm_value;
	}
}

char vvvfTwoLevel(CalculateParam *param)
{

	VvvfValues *status = param->status;
	PwmCalculateValues *pwm = param->pwm;
	double initialPhase = param->initialPhase;

	// Set to minimum frequency
	if(status->sin_angle_freq < pwm->min_freq * M_2PI && status->wave_stat > 0){
		status->v_sin_angle_freq = pwm->min_freq * M_2PI;
		status->allow_sine_time_change = false;
	}
	else{
		status->v_sin_angle_freq = status->sin_angle_freq;
		status->allow_sine_time_change = true;
	}

	if (pwm->none) return 0;

	double _sin_x = status->v_sin_angle_freq * status->sin_time + initialPhase;
	double amplitude = pwm->amplitude;

	switch ((int)pwm->pulse_mode.pulse_name)
	{
	case P_Wide_3:
		return getWideP3(_sin_x, amplitude, false);
	case SP_Wide_3:
		return getWideP3(_sin_x, amplitude, true);
	case P_5:
		return getPulseWithSaw(_sin_x, amplitude, getPulseNum(pwm->pulse_mode.pulse_name), false);
	case SP_5:
		return getPulseWithSaw(_sin_x, amplitude, getPulseNum(pwm->pulse_mode.pulse_name), true);
	case P_7:
		return getPulseWithSaw(_sin_x, amplitude, getPulseNum(pwm->pulse_mode.pulse_name), false);
	case SP_7:
		return getPulseWithSaw(_sin_x, amplitude, getPulseNum(pwm->pulse_mode.pulse_name), true);
	case P_11:
		return getPulseWithSaw(_sin_x, amplitude, getPulseNum(pwm->pulse_mode.pulse_name), false);
	case SP_11:
		return getPulseWithSaw(_sin_x, amplitude, getPulseNum(pwm->pulse_mode.pulse_name), true);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_15)
	{
		int array_l = (int)(1000 * amplitude) + 1;
		return getPulseWithSwitchAngle(
			_7Alpha[array_l][0] * M_PI_180,
			_7Alpha[array_l][1] * M_PI_180,
			_7Alpha[array_l][2] * M_PI_180,
			_7Alpha[array_l][3] * M_PI_180,
			_7Alpha[array_l][4] * M_PI_180,
			_7Alpha[array_l][5] * M_PI_180,
			_7Alpha[array_l][6] * M_PI_180,
			_7Alpha_Polary[array_l], _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_Old_15)
	{
		int array_l = (int)(1000 * amplitude) + 1;
		return getPulseWithSwitchAngle(
			_7Alpha_Old[array_l][0] * M_PI_180,
			_7Alpha_Old[array_l][1] * M_PI_180,
			_7Alpha_Old[array_l][2] * M_PI_180,
			_7Alpha_Old[array_l][3] * M_PI_180,
			_7Alpha_Old[array_l][4] * M_PI_180,
			_7Alpha_Old[array_l][5] * M_PI_180,
			_7Alpha_Old[array_l][6] * M_PI_180,
			_7OldAlpha_Polary[array_l], _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_Wide_15)
	{
		int array_l = (int)(1000 * amplitude) - 999;
		return getPulseWithSwitchAngle(
			_7WideAlpha[array_l][0] * M_PI_180,
			_7WideAlpha[array_l][1] * M_PI_180,
			_7WideAlpha[array_l][2] * M_PI_180,
			_7WideAlpha[array_l][3] * M_PI_180,
			_7WideAlpha[array_l][4] * M_PI_180,
			_7WideAlpha[array_l][5] * M_PI_180,
			_7WideAlpha[array_l][6] * M_PI_180,
			'B', _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_13)
	{
		int array_l = (int)(1000 * amplitude) + 1;
		return getPulseWithSwitchAngle(
			_6Alpha[array_l][0] * M_PI_180,
			_6Alpha[array_l][1] * M_PI_180,
			_6Alpha[array_l][2] * M_PI_180,
			_6Alpha[array_l][3] * M_PI_180,
			_6Alpha[array_l][4] * M_PI_180,
			_6Alpha[array_l][5] * M_PI_180,
			M_PI_2,
			_6Alpha_Polary[array_l], _sin_x);
	}
	if (pwm->pulse_mode.pulse_name == CHMP_Old_13)
	{
		int array_l = (int)(1000 * amplitude) + 1;
		return getPulseWithSwitchAngle(
			_6Alpha_Old[array_l][0] * M_PI_180,
			_6Alpha_Old[array_l][1] * M_PI_180,
			_6Alpha_Old[array_l][2] * M_PI_180,
			_6Alpha_Old[array_l][3] * M_PI_180,
			_6Alpha_Old[array_l][4] * M_PI_180,
			_6Alpha_Old[array_l][5] * M_PI_180,
			M_PI_2,
			_6OldAlpha_Polary[array_l], _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_Wide_13)
	{
		int array_l = (int)(1000 * amplitude) - 999;
		return getPulseWithSwitchAngle(
			_6WideAlpha[array_l][0] * M_PI_180,
			_6WideAlpha[array_l][1] * M_PI_180,
			_6WideAlpha[array_l][2] * M_PI_180,
			_6WideAlpha[array_l][3] * M_PI_180,
			_6WideAlpha[array_l][4] * M_PI_180,
			_6WideAlpha[array_l][5] * M_PI_180,
			M_PI_2,
			'A', _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_11)
	{
		int array_l = (int)(1000 * amplitude) + 1;
		return getPulseWithSwitchAngle(
			_5Alpha[array_l][0] * M_PI_180,
			_5Alpha[array_l][1] * M_PI_180,
			_5Alpha[array_l][2] * M_PI_180,
			_5Alpha[array_l][3] * M_PI_180,
			_5Alpha[array_l][4] * M_PI_180,
			M_PI_2,
			M_PI_2,
			_5Alpha_Polary[array_l], _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_Old_11)
	{
		int array_l = (int)(1000 * amplitude) + 1;
		return getPulseWithSwitchAngle(
			_5Alpha_Old[array_l][0] * M_PI_180,
			_5Alpha_Old[array_l][1] * M_PI_180,
			_5Alpha_Old[array_l][2] * M_PI_180,
			_5Alpha_Old[array_l][3] * M_PI_180,
			_5Alpha_Old[array_l][4] * M_PI_180,
			M_PI_2,
			M_PI_2,
			_5OldAlpha_Polary[array_l], _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_Wide_11)
	{
		int array_l = (int)(1000 * amplitude) - 999;
		return getPulseWithSwitchAngle(
			_5WideAlpha[array_l][0] * M_PI_180,
			_5WideAlpha[array_l][1] * M_PI_180,
			_5WideAlpha[array_l][2] * M_PI_180,
			_5WideAlpha[array_l][3] * M_PI_180,
			_5WideAlpha[array_l][4] * M_PI_180,
			M_PI_2,
			M_PI_2,
			'B', _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_9)
	{
		int array_l = (int)(1000 * amplitude) + 1;
		return getPulseWithSwitchAngle(
			_4Alpha[array_l][0] * M_PI_180,
			_4Alpha[array_l][1] * M_PI_180,
			_4Alpha[array_l][2] * M_PI_180,
			_4Alpha[array_l][3] * M_PI_180,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			_4Alpha_Polary[array_l], _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_Wide_9)
	{
		int array_l = (int)(1000 * amplitude) - 799;
		return getPulseWithSwitchAngle(
			_4WideAlpha[array_l][0] * M_PI_180,
			_4WideAlpha[array_l][1] * M_PI_180,
			_4WideAlpha[array_l][2] * M_PI_180,
			_4WideAlpha[array_l][3] * M_PI_180,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			'A', _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_7)
	{
		int array_l = (int)(1000 * amplitude) + 1;
		return getPulseWithSwitchAngle(
			_3Alpha[array_l][0] * M_PI_180,
			_3Alpha[array_l][1] * M_PI_180,
			_3Alpha[array_l][2] * M_PI_180,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			_3Alpha_Polary[array_l], _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_Wide_7)
	{
		int array_l = (int)(1000 * amplitude) - 799;
		return getPulseWithSwitchAngle(
			_3WideAlpha[array_l][0] * M_PI_180,
			_3WideAlpha[array_l][1] * M_PI_180,
			_3WideAlpha[array_l][2] * M_PI_180,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			'B', _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_5)
	{
		int array_l = (int)(1000 * amplitude) + 1;
		return getPulseWithSwitchAngle(
			_2Alpha[array_l][0] * M_PI_180,
			_2Alpha[array_l][1] * M_PI_180,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			_2Alpha_Polary[array_l], _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_Wide_5)
	{
		int array_l = (int)(1000 * amplitude) - 799;
		return getPulseWithSwitchAngle(
			_2WideAlpha[array_l][0] * M_PI_180,
			_2WideAlpha[array_l][1] * M_PI_180,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			'A', _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == CHMP_Wide_3)
	{
		return getPulseWithSwitchAngle(
			_WideAlpha[(int)(500 * amplitude) + 1] * M_PI_180,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			'B', _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == SHEP_3)
	{
		return getPulseWithSwitchAngle(
			_1Alpha_SHE[(int)(1000 * amplitude) + 1] * M_PI_180,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			'B', _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == SHEP_5)
	{
		int array_l = (int)(1000 * amplitude) + 1;
		return getPulseWithSwitchAngle(
			_2Alpha_SHE[array_l][0] * M_PI_180,
			_2Alpha_SHE[array_l][1] * M_PI_180,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			'A', _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == SHEP_7)
	{
		int array_l = (int)(1000 * amplitude) + 1;
		return getPulseWithSwitchAngle(
			_3Alpha_SHE[array_l][0] * M_PI_180,
			_3Alpha_SHE[array_l][1] * M_PI_180,
			_3Alpha_SHE[array_l][2] * M_PI_180,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			M_PI_2,
			'B', _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == SHEP_11)
	{
		int array_l = (int)(1000 * amplitude) + 1;
		return getPulseWithSwitchAngle(
			_5Alpha_SHE[array_l][0] * M_PI_180,
			_5Alpha_SHE[array_l][1] * M_PI_180,
			_5Alpha_SHE[array_l][2] * M_PI_180,
			_5Alpha_SHE[array_l][3] * M_PI_180,
			_5Alpha_SHE[array_l][4] * M_PI_180,
			M_PI_2,
			M_PI_2,
			'A', _sin_x);
	}

	if (pwm->pulse_mode.pulse_name == Async || pwm->pulse_mode.pulse_name == Async_THI)
	{
		double carrier_angle_freq = solveCarrierFrequency(status, pwm) * M_2PI;
		status->saw_time *= status->saw_angle_freq / carrier_angle_freq;
		status->saw_angle_freq = carrier_angle_freq;

		double sin_value;
		if (pwm->pulse_mode.pulse_name == Async_THI)
			sin_value = getSin(_sin_x, amplitude) + 0.2 * getSin(3 * _sin_x, amplitude);
		else
			sin_value = getSin(_sin_x, amplitude);

		double saw_value = getSaw(status->saw_time * status->saw_angle_freq + 0);
		char pwm_value = modulateSin(sin_value, saw_value) << 1;

		return pwm_value;
	}

	else
	{
		status->saw_time = status->sin_time;
		status->saw_angle_freq = status->v_sin_angle_freq;

		int pulse = getPulseNum(pwm->pulse_mode.pulse_name);

		double sin_value = getSin(_sin_x, amplitude);
		double saw_value = getSaw(pulse * _sin_x);
		if ((int)pwm->pulse_mode.pulse_name > (int)P_61)
			saw_value = -saw_value;

		char pwm_value = modulateSin(sin_value, saw_value) << 1;
		return pwm_value;
	}
}
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "rpi-smartstart.h"
#include "QA7.h"
#include "xRTOS.h"
#include "task.h"
#include "windows.h"
#include "semaphore.h"

#include "vvvf-hardware.h"
#include "vvvf-sounds.h"
#include "vvvf-struct.h"
#include "vvvf-main.h"
#include "pi-values.h"
#include "icon.h"
#include "character.h"

#include "dma.h"

void screenDMA(uint32_t source_addr, uint32_t dest_addr, uint32_t length)
{
	static DMA_ControlBlock dma_control_block = {0};
	DMA_CH_0.control_status |= 1 << 31; /* set reset */
	dma_control_block.transfer_information = 0;
	dma_control_block.transfer_information |= 1 << 26 | 1 << 9 | 1 << 8 | 1 << 5 | 1 << 4 | 1 << 0; /* no wide bursts, src increment, dest increment, interrupt bit */
	dma_control_block.source_addr = source_addr;
	dma_control_block.dest_addr = dest_addr;
	dma_control_block.next_control_block = 0;
	dma_control_block.stride = 0;
	dma_control_block.transfer_length = length;
	dma_control_block.dummy0 = 0;
	dma_control_block.dummy1 = 0;
	DMA_CH_0.control_status |= 1 << 29;
	DMA_CH_0.control_block_addr = (uint32_t)&dma_control_block | 0xC0000000;
	DMA_CH_0.control_status |= 1 << 0;
	while (!(DMA_CH_0.control_status & (1 << 2)))
		;
	DMA_CH_0.control_status |= 1 << 2;
}

void initializeDMA()
{
	DMA_ENABLE |= 1 << 0; /* enable DMA ch 0 */
}

void DrawRoundRect(uint16_t *buff, int R, int x, int y, int width, int height, uint16_t col)
{
	{ // Fill easy area.
		int _fill_x = width - 2 * R;
		int _fill_y = height;
		while (_fill_y > 0)
		{
			_fill_y--;
			_fill_x = width - 2 * R;
			while (_fill_x > 0)
			{
				_fill_x--;
				buff[(y + _fill_y) * frameBufferWidth + x + R + _fill_x] = col;
			}
		}
	}

	for (int i = 0; i <= R; i++)
	{ // ROUND of 20 px
		// LEFT TOP
		int circleL = (int)round(sqrt(-i * i + 2 * R * i));
		int circleR = (int)round(sqrt(R * R - i * i));

		int _fill_y = height - 2 * (R - circleL);
		while (_fill_y > 0)
		{
			_fill_y--;
			buff[(y + R - circleL + _fill_y) * frameBufferWidth + x + i] = col;
		}

		_fill_y = height - 2 * (R - circleR);
		while (_fill_y > 0)
		{
			_fill_y--;
			buff[(y + R - circleR + _fill_y) * frameBufferWidth + x + width - R + i] = col;
		}
	}
}

void DrawMonoCharacter(uint16_t *buff, long *data, int width, int height, int x, int y, uint16_t col, uint16_t fill)
{
	int _t = 0;
	while (_t < width * height)
	{
		int _index = _t / 32;
		int _bit = _t % 32;
		int _x = _t % width;
		int _y = _t / width;
		if (((data[_index] << _bit) & 0x80000000))
		{
			buff[x + _x + frameBufferWidth * (y + _y)] = col;
		}
		else
		{
			if (fill != 0){
				buff[x + _x + frameBufferWidth * (y + _y)] = fill;
			}
		}
		_t++;
	}
}

void DrawNumber(uint16_t *buff, int value, int x, int y, int width, int height, int max_digit, bool zero_fill, bool center, uint16_t col, uint16_t fill, uint16_t fill_space)
{
	int original_x = x;
	int digit = zero_fill ? max_digit : (value <= 0 ? 1 : (log10(value) + 1));
	if (center)
	{
		x += (width - character_num_width * digit) / 2;
		y += (height - character_num_height) / 2;
	}

	if (fill_space != 0)
	{
		int temp_x = 0, temp_y = y;
		while (temp_y < y + character_num_height)
		{
			temp_x = original_x;
			while (temp_x < x)
			{
				buff[temp_x + frameBufferWidth * (temp_y)] = fill_space;
				temp_x++;
			}
			temp_x = x + character_num_width * digit;
			while (temp_x < original_x + width)
			{
				buff[temp_x + frameBufferWidth * (temp_y)] = fill_space;
				temp_x++;
			}
			temp_y++;
		}
	}

	for (int i = 0; i < digit; i++)
	{
		DrawMonoCharacter(buff, (long *)character_num[value % 10], character_num_width, character_num_height, x + (digit - i - 1) * character_num_width, y, col, fill);
		value /= 10;
	}
}

#define COLOR_BLANK 0x2965
#define COLOR_BLANK_TEXT 0xE71C
#define COLOR_WAVEFORM 0xFFCB
#define COLOR_WAVEFORM_BASE 0x9492
#define COLOR_WAVEFORM_FILL 0xE75F
#define COLOR_WAVEFORM_BACKGROUND COLOR_BLANK
#define COLOR_KEY COLOR_WAVEFORM
#define COLOR_KEY_TEXT_BACK COLOR_BLANK
#define COLOR_KEY_TEXT 0xfe46
#define COLOR_KEY_TEXT_SUB COLOR_BLANK_TEXT
#define COLOR_KEY_BORDER 0xFFFF

void initializeGUI(uint16_t *buff)
{
	int _x = frameBufferWidth;
	int _y = frameBufferHeight;

	while (_x > 0)
	{
		_x--;
		_y = frameBufferHeight;
		while (_y > 0)
		{
			_y--;
			buff[_x + frameBufferWidth * _y] = COLOR_BLANK;
		}
	}

	DrawRoundRect(buff, 20, 5, 201, frameBufferWidth - 10, 140, COLOR_KEY_BORDER);
	DrawRoundRect(buff, 20, 6, 202, frameBufferWidth - 12, 138, COLOR_KEY_TEXT_BACK);
	DrawMonoCharacter(buff, (long *)character_voltage, 190, 80, 10, 200, COLOR_KEY_TEXT_SUB, 0x00);
	DrawMonoCharacter(buff, (long *)character_frequency, 190, 80, 10, 260, COLOR_KEY_TEXT_SUB, 0x00);
	DrawMonoCharacter(buff, (long *)character_percent, 40, 20, 470, 250, COLOR_KEY_TEXT_SUB, 0x00);
	DrawMonoCharacter(buff, (long *)character_hz, 40, 20, 470, 310, COLOR_KEY_TEXT_SUB, 0x00);
}

void taskMascon(void *param)
{
	const char vvvf_sound_len = vvvf_sounds_len;
	char current_vvvf_sound = 0;
	const double freqAdd = 1; // [Hz / sec]
	uint64_t _dt = 0, _s;
	while (1)
	{
		_s = timer_getTickCount64();

		signed char mascon = (signed char)readMasconValue() - 4;
		requestStatusVvvfGpio();
		while (!canGetStatusVvvfGpio())
			;
		VvvfValues gpioStatus = getStatusVvvfGpio();

		if (!readButtonR() && gpioStatus.sin_angle_freq == 0)
		{
			current_vvvf_sound++;
			if (current_vvvf_sound >= vvvf_sound_len)
				current_vvvf_sound = 0;
			setSoundVvvfGpio(vvvf_sounds[(int)(current_vvvf_sound)]);
			while (!readButtonR())
				;
		}

		double _changedFreq = gpioStatus.wave_stat + mascon * freqAdd * _dt / 1000000.0;
		if (_changedFreq < 0)
			_changedFreq = 0;
		if (_changedFreq > 100)
			_changedFreq = 100;

		if (mascon != 0)
		{
			gpioStatus.brake = mascon > 0 ? 0 : 1;
			gpioStatus.mascon_off = false;
		}
		else
			gpioStatus.mascon_off = true;

		double gpioStatusOldAngleFreq = gpioStatus.sin_angle_freq;
		if (!gpioStatus.free_run)
		{
			gpioStatus.wave_stat = _changedFreq;
			gpioStatus.sin_angle_freq = _changedFreq * M_2PI;
			if (gpioStatus.allow_sine_time_change)
				gpioStatus.sin_time *= ((gpioStatus.sin_angle_freq == 0) ? 1 : (gpioStatusOldAngleFreq / gpioStatus.sin_angle_freq));
		}

		setStatusVvvfGpio(&gpioStatus);

		while (timer_getTickCount64() - _s < 5000)
			;
		_dt = timer_getTickCount64() - _s;
	}
}
#ifdef ENABLE_DISPLAY

int screenBufferRenderPos = 0;
uint16_t screenBuffer[640*360];
char screenBufferWait = 0;

void processScreenBufferRender(){
	if(screenBufferWait == 0) return;
	*(uint16_t *)((frameBufferAddress & ~0xC0000000) + screenBufferRenderPos * 2) = screenBuffer[screenBufferRenderPos];
	screenBufferRenderPos ++;
	if(screenBufferRenderPos >= 640*360) {
		screenBufferRenderPos = 0;
		screenBufferWait = 0;
	}
}

void taskDisplay(void *param)
{
	int max_i = frameBufferWidth;
	signed char buff[max_i];
	static int waveFormImgDispX = 0;
	

	initializeGUI(screenBuffer);

	while (1)
	{
		requestStatusVvvfGpio();
		while (!canGetStatusVvvfGpio())
			;
		VvvfValues displayStatus = getStatusVvvfGpio();
		VvvfSoundFunction gpioSound = (VvvfSoundFunction)getSoundVvvfGpio();
		displayStatus.sin_time = 0;
		displayStatus.saw_time = 0;
		displayStatus.allow_random_freq_move = false;

		for (int i = 0; i < max_i; i++)
		{
			displayStatus.sin_time = (double)i / max_i / 30.0;
			displayStatus.saw_time = (double)i / max_i / 30.0;
			displayStatus.generation_current_time = (double)i / max_i / 30.0;
			PhaseStatus U, V;
			calculatePhases(&U, &V, 0, &displayStatus, gpioSound);
			buff[i] = (signed char)V - (signed char)U;
		}

		displayStatus.sin_time = 0;
		displayStatus.saw_time = 0;
		double b_1 = 0;
		for (int i = 0; i < 5000; i++)
		{
			displayStatus.sin_time = (double)i / (5000 * displayStatus.v_sin_angle_freq * M_1_2PI);
			displayStatus.saw_time = (double)i / (5000 * displayStatus.v_sin_angle_freq * M_1_2PI);
			displayStatus.generation_current_time = (double)i / (5000 * displayStatus.v_sin_angle_freq * M_1_2PI);

			PhaseStatus U, V;
			calculatePhases(&U, &V, 0, &displayStatus, gpioSound);
			signed char val = (signed char)U - (signed char)V;
			b_1 += val * sin(M_2PI * i / 5000);
		}
		b_1 /= 1.1026577908425 * 5000;
		b_1 *= 100;

		DrawNumber(screenBuffer, (int)(round(displayStatus.v_sin_angle_freq * M_1_2PI)), 200, 261, 270, 80, 3, false, true, COLOR_KEY_TEXT, COLOR_KEY_TEXT_BACK, COLOR_KEY_TEXT_BACK); // BOX-1 CONTENT
		DrawNumber(screenBuffer, (int)b_1, 200, 201, 270, 80, 3, false, true, COLOR_KEY_TEXT, COLOR_KEY_TEXT_BACK, COLOR_KEY_TEXT_BACK);				  // BOX-2 CONTENT

		waveFormImgDispX = 0;
		while (waveFormImgDispX < max_i)
		{
			// Background Fill
			int bg_fill = 0;
			while (bg_fill <= 200)
			{
				screenBuffer[0 + waveFormImgDispX + 640 * (0 + bg_fill)] = COLOR_WAVEFORM_BACKGROUND;
				bg_fill++;
			}

			// Base Line
			screenBuffer[0 + waveFormImgDispX + 640 * (100)] = COLOR_WAVEFORM_BASE;

			// Draw graph on x = i
			int y_diff = (buff[waveFormImgDispX] - buff[(waveFormImgDispX + 1) == max_i ? waveFormImgDispX : waveFormImgDispX + 1]) * 40;
			int abs_y_diff = y_diff < 0 ? -y_diff : y_diff;
			int y_start = buff[waveFormImgDispX] * 40 + 100;

			// int y_fill = buff[waveFormImgDispX] * 40;
			// int y_fill_dir = buff[waveFormImgDispX] > 0 ? -1 : 1;
			// y_fill *= y_fill_dir;
			// while (y_fill < 0)
			// {
			// 	y_fill++;
			// 	screenBuffer[0 + waveFormImgDispX + 640 * (100 + y_fill * y_fill_dir)] = COLOR_WAVEFORM_FILL;
			// }

			while (abs_y_diff >= 0)
			{
				screenBuffer[0 + waveFormImgDispX + 640 * (y_start + abs_y_diff * (y_diff > 0 ? -1 : 1))] = COLOR_WAVEFORM;
				abs_y_diff--;
			}

			waveFormImgDispX++;
		}

		screenBufferWait = 1;
		timer_wait(50000);
	}
}
#endif

void noTask(void *param)
{
	while (1)
	{
		__asm__("nop;\n");
	}
}

void flickTask(void *param)
{
	while (1)
	{
		Flash_Debug1();
		timer_wait(2000);
	}
}

void main(void)
{
	Flash_LED();
	initializeVvvfHardware(); // Initialize vvvf relating pins
	setSoundVvvfGpio(vvvf_sounds[0]);

#ifdef ENABLE_DISPLAY
	initializeWindow(640, 360, 1024, 600, 16); // Auto resolution console, message to screen
	initializeDMA();
	screenDMA((uint32_t)boot_img, frameBufferAddress & ~0xC0000000, 640 * 360 * 2);
#endif

	xRTOS_Init(); // Initialize the xRTOS system .. done before any other xRTOS call
#ifdef ENABLE_DISPLAY
	xTaskCreate(0, taskDisplay, "Disp", 1024, NULL, 1, NULL);
#else
	xTaskCreate(0, noTask, "DispNop", 1024, NULL, 1, NULL);
#endif
	xTaskCreate(1, taskCalculationPhases, "Vvvf", 1024, NULL, 1, NULL);
	xTaskCreate(2, taskMascon, "Mascon", 1024, NULL, 1, NULL);
	xTaskCreate(3, noTask, "3-nop", 1024, NULL, 1, NULL);

	Flash_LED();
	timer_wait(5000000);
	Flash_LED();

	xTaskStartScheduler();
}

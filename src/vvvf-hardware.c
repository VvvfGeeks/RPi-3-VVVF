#include "vvvf-hardware.h"
#include <stdbool.h>
#include <stdint.h>

#include "rpi-smartstart.h"

/*-[gpio_outputs]------------------------------------------------------------}
. Given a valid GPIO port number the output is set high(true) or Low (false)
. Avail range for GPIO is 0 to 31
.--------------------------------------------------------------------------*/
bool gpio_outputs (uint32_t mask_bit, bool on) 
{
	volatile uint32_t* p = (uint32_t*)(RPi_IO_Base_Addr + 0x200000 + (on ?  0x1C : 0x28));
    *p = mask_bit;									        // Output bit to register number	
    return true;												// Return true
}


static bool lit = false;
void Flash_LED(void)
{
	if (lit) lit = false; else lit = true;							// Flip lit flag
	set_Activity_LED(lit);											// Turn LED on/off as per new flag
}

/**
 * @brief 
 * This function must implement initialize all of pins and when it is set output mode, it will set to 0;
 */
void initializeVvvfHardware(){
    gpio_setup(PIN_U_HIGH_2, GPIO_OUTPUT);                          // Initialize U Pins
    gpio_output(PIN_U_HIGH_2, false);
    gpio_setup(PIN_U_HIGH_1, GPIO_OUTPUT);
    gpio_output(PIN_U_HIGH_1, false);
    gpio_setup(PIN_U_LOW_1, GPIO_OUTPUT);
    gpio_output(PIN_U_LOW_1, false);
    gpio_setup(PIN_U_LOW_2, GPIO_OUTPUT);
    gpio_output(PIN_U_LOW_2, false);

    gpio_setup(PIN_V_HIGH_2, GPIO_OUTPUT);                          // Initialize V Pins
    gpio_output(PIN_V_HIGH_2, false);
    gpio_setup(PIN_V_HIGH_1, GPIO_OUTPUT);
    gpio_output(PIN_V_HIGH_1, false);
    gpio_setup(PIN_V_LOW_1, GPIO_OUTPUT);
    gpio_output(PIN_V_LOW_1, false);
    gpio_setup(PIN_V_LOW_2, GPIO_OUTPUT);
    gpio_output(PIN_V_LOW_2, false);

    gpio_setup(PIN_W_HIGH_2, GPIO_OUTPUT);                          // Initialize W Pins
    gpio_output(PIN_W_HIGH_2, false);
    gpio_setup(PIN_W_HIGH_1, GPIO_OUTPUT);
    gpio_output(PIN_W_HIGH_1, false);
    gpio_setup(PIN_W_LOW_1, GPIO_OUTPUT);
    gpio_output(PIN_W_LOW_1, false);
    gpio_setup(PIN_W_LOW_2, GPIO_OUTPUT);
    gpio_output(PIN_W_LOW_2, false);

	gpio_setup(PIN_MASCON_BIT0, GPIO_INPUT);                        // Initialize Mascon Pins
	gpio_setup(PIN_MASCON_BIT1, GPIO_INPUT);
	gpio_setup(PIN_MASCON_BIT2, GPIO_INPUT);
	gpio_setup(PIN_MASCON_BIT3, GPIO_INPUT);

	gpio_setup(PIN_BTN_R, GPIO_INPUT);                              // Initialize PIN btn Pins
	gpio_setup(PIN_BTN_SEL, GPIO_INPUT);
	gpio_setup(PIN_BTN_L, GPIO_INPUT);

	gpio_setup(PIN_DEBUG_1, GPIO_OUTPUT);                           // Initialize Debug pins
    gpio_output(PIN_DEBUG_1, false);
	gpio_setup(PIN_DEBUG_2, GPIO_OUTPUT);
    gpio_output(PIN_DEBUG_2, false);
}

static bool _lit_debug_1 = false;
void Flash_Debug1(void)
{
#ifdef PIN_DEBUG_1_ENABLE
	_lit_debug_1 = !_lit_debug_1;											// Flip _lit_debug_1 flag
	gpio_output(PIN_DEBUG_1, _lit_debug_1);									// Turn LED on/off as per new flag
#endif
}

static bool _lit_debug_2 = false;
void Flash_Debug2(void)
{
#ifdef PIN_DEBUG_2_ENABLE
	_lit_debug_2 = !_lit_debug_2;											// Flip _lit_debug_2 flag
	gpio_output(PIN_DEBUG_2, _lit_debug_2);									// Turn LED on/off as per new flag
#endif
}

/**
 * @brief 
 * Reads mascon value and convert to char
 * 
 * @return char 
 */
char readMasconValue(){
    char status = (char)gpio_input(PIN_MASCON_BIT0) | (char)(gpio_input(PIN_MASCON_BIT1) << 1) | (char)(gpio_input(PIN_MASCON_BIT2) << 2) | (char)(gpio_input(PIN_MASCON_BIT3) << 3);
	return status;
}

/**
 * @brief 
 * Read Btn status R
 */
char readButtonR(){
    return (char)gpio_input(PIN_BTN_R);
}

/**
 * @brief 
 * Read Btn status SEL
 */
char readButtonSel(){
    return (char)gpio_input(PIN_BTN_SEL);
}

/**
 * @brief 
 * Read Btn status L
 */
char readButtonL(){
    return (char)gpio_input(PIN_BTN_L);
}

/**
 * @brief Create the PhasePinStatus object
 *                              H_2 H_1 L_1 L_2
 * stat = PHASE_LOW         :   0   0   1   1
 * stat = PHASE_MIDDLE      :   0   1   1   0
 * stat = PHASE_HIGH        :   1   1   0   0
 * 
 * @param stat 
 * @return PhasePinStatus 
 */
PhasePinStatus createPhasePinStatus(PhaseStatus status){
    PhasePinStatus pin_status = {
        status == PHASE_HIGH,
        status == PHASE_HIGH || status == PHASE_MIDDLE,
        status == PHASE_MIDDLE || status == PHASE_LOW,
        status == PHASE_LOW
    };
    return pin_status;
}

/**
 * @brief Sets pin status for each Phase
 *                              H_2 H_1 L_1 L_2
 * stat = PHASE_LOW         :   0   0   1   1
 * stat = PHASE_MIDDLE      :   0   1   1   0
 * stat = PHASE_HIGH        :   1   1   0   0
 * 
 * @param stat 
 * @return void 
 */
void setPhasePinStatus(PhaseStatus U, PhaseStatus V, PhaseStatus W){
    PhasePinStatus _U = createPhasePinStatus(U);
    PhasePinStatus _V = createPhasePinStatus(V);
    PhasePinStatus _W = createPhasePinStatus(W);

    uint32_t high = 0, low = 0;

    // U PHASE
    *(_U.H_2 ? &high : &low) |= 1 << PIN_U_HIGH_2;
    *(_U.H_1 ? &high : &low) |= 1 << PIN_U_HIGH_1;
    *(_U.L_1 ? &high : &low) |= 1 << PIN_U_LOW_1;
    *(_U.L_2 ? &high : &low) |= 1 << PIN_U_LOW_2;

    // V PHASE
    *(_V.H_2 ? &high : &low) |= 1 << PIN_V_HIGH_2;
    *(_V.H_1 ? &high : &low) |= 1 << PIN_V_HIGH_1;
    *(_V.L_1 ? &high : &low) |= 1 << PIN_V_LOW_1;
    *(_V.L_2 ? &high : &low) |= 1 << PIN_V_LOW_2;

    // W PHASE
    *(_W.H_2 ? &high : &low) |= 1 << PIN_W_HIGH_2;
    *(_W.H_1 ? &high : &low) |= 1 << PIN_W_HIGH_1;
    *(_W.L_1 ? &high : &low) |= 1 << PIN_W_LOW_1;
    *(_W.L_2 ? &high : &low) |= 1 << PIN_W_LOW_2;

    gpio_outputs(low, false);
    gpio_outputs(high, true);
}

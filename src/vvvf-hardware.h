#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

//
// Here's definition for each pin.
//

#define PIN_U_HIGH_2 12
#define PIN_U_HIGH_1 13
#define PIN_U_LOW_1 11
#define PIN_U_LOW_2 21

#define PIN_V_HIGH_2 16
#define PIN_V_HIGH_1 6
#define PIN_V_LOW_1 9
#define PIN_V_LOW_2 26

#define PIN_W_HIGH_2 20
#define PIN_W_HIGH_1 5
#define PIN_W_LOW_1 10
#define PIN_W_LOW_2 19

#define PIN_MASCON_BIT0 4
#define PIN_MASCON_BIT1 17
#define PIN_MASCON_BIT2 27
#define PIN_MASCON_BIT3 22

#define PIN_BTN_R 7
#define PIN_BTN_SEL 8
#define PIN_BTN_L 25

#define PIN_DEBUG_2 23
#define PIN_DEBUG_1 24

#define PIN_DEBUG_2_ENABLE
#define PIN_DEBUG_1_ENABLE

#endif

#ifndef VVVF_RASPBERRYPI_H
#define VVVF_RASPBERRYPI_H

typedef char PhaseStatus;
#define PHASE_HIGH 2
#define PHASE_MIDDLE 1
#define PHASE_LOW 0

typedef struct {
    char H_2;
    char H_1;
    char L_1;
    char L_2;
} PhasePinStatus;

/**
 * @brief 
 * Blink Default LED
 */
void Flash_LED(void);

/**
 * @brief 
 * This function must implement initialize all of pins and when it is set output mode, it will set to 0;
 */
void initializeVvvfHardware(void);

/**
 * @brief 
 * Blink debug 1 pin
 */
void Flash_Debug1(void);

/**
 * @brief 
 * Blink debug 2 pin
 */
void Flash_Debug2(void);

/**
 * @brief 
 * Reads mascon value and convert to char
 * 
 * @return char 
 */
char readMasconValue(void);

/**
 * @brief 
 * Read Btn status R
 */
char readButtonR();

/**
 * @brief 
 * Read Btn status SEL
 */
char readButtonSel();

/**
 * @brief 
 * Read Btn status L
 */
char readButtonL();

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
PhasePinStatus createPhasePinStatus(PhaseStatus);

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
void setPhasePinStatus(PhaseStatus, PhaseStatus, PhaseStatus);

#endif
#ifndef __BESCONTROLS_H__
#define __BESCONTROLS_H__

/* These are the functions that map inputs from the various input
  methods (USB gamepad, GPIO, and PRU SNES gamepads) into the 
  keyboard events used by the various emulators. */

/* Enumeration for the number of players */
enum {
  PLAYER_INVALID = -1,
  PLAYER_FIRST = 0,
  PLAYER_ONE = PLAYER_FIRST,
  PLAYER_TWO,
  PLAYER_LAST = PLAYER_TWO,
  PLAYER_TOTAL
};

/* Enumeration for gamepad buttons */
typedef enum {
  BUTTON_INVALID = -1,
  BUTTON_FIRST = 0,
  BUTTON_L = BUTTON_FIRST,
  BUTTON_R,
  BUTTON_A,
  BUTTON_B,
  BUTTON_X,
  BUTTON_Y,
  BUTTON_START,
  BUTTON_SELECT,
  BUTTON_PAUSE,
  BUTTON_LAST = BUTTON_PAUSE,
  BUTTON_TOTAL
} enumButton_t;

/* Enumeration for gamepad axes */
typedef enum {
  AXIS_INVALID = -1,
  AXIS_FIRST = 0,
  AXIS_VERTICAL = AXIS_FIRST,
  AXIS_HORIZONTAL,
  AXIS_LAST = AXIS_HORIZONTAL,
  AXIS_TOTAL
} enumAxis_t;

/* Enumeration for gamepad axis settings */
typedef enum {
  AXISSETTING_INVALID = -1,
  AXISSETTING_FIRST = 0,
  AXISSETTING_INVERT = AXISSETTING_FIRST,
  AXISSETTING_DEADZONE,
  AXISSETTING_LAST = AXISSETTING_DEADZONE,
  AXISSETTING_TOTAL
} enumAxisSetting_t;

/* Enumeration for GPIO button input */
typedef enum {
  GPIO_INVALID = -1,
  GPIO_FIRST = 0,
  GPIO_GPLEFT = GPIO_FIRST,
  GPIO_GPRIGHT,
  GPIO_GPUP,
  GPIO_GPDOWN,
  GPIO_LEFT,
  GPIO_RIGHT,
  GPIO_A,
  GPIO_B,
  GPIO_X,
  GPIO_Y,
  GPIO_START,
  GPIO_SELECT,
  GPIO_PAUSE,
  GPIO_LAST = GPIO_PAUSE,
  GPIO_TOTAL
} enumGpio_t;


/* Setup/shutdown/process PRU and GPIO inputs */
extern void BESControlSetup(void);
extern void BESControlShutdown(void);
extern void BESProcessEvents(void);
extern void BESGPIOToggle(const int gpio, const int value);
extern uint8_t BESGPIOMap[GPIO_TOTAL];
extern uint32_t BESPRUGamepadPresent[PLAYER_TOTAL];

/* Setup/shutdown/process USB gamepad inputs */
extern void BESResetJoysticks(void);
extern void BESCheckJoysticks(void);
extern void BESProcessJoystickEvent(const SDL_Event *event);
extern uint8_t BESJoystickButtonMap[PLAYER_TOTAL][BUTTON_TOTAL + AXIS_TOTAL];
extern uint8_t BESJoystickAxisSettingMap[PLAYER_TOTAL][AXIS_TOTAL][AXISSETTING_TOTAL];
extern uint32_t BESJoystickPresent[PLAYER_TOTAL];

/* Bitmask of buttons that trigger the pause combo */
extern uint32_t BESPauseCombo;
 
#endif /* __BESCONTROLS */

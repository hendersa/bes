#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <SDL.h>

#include "beagleboard.h"
#include "besKeys.h"
#include "besPru.h"
#include "besControls.h"

/* ---- START: PRU constants/variables/functions ---- */
#define PRU_BITS 12
static uint32_t lastPRUSample = 0xFFFFFFFF;
static uint32_t pruEnabled = 0;
uint32_t BESPRUGamepadPresent[PLAYER_TOTAL] = {1, 1};

/* Map of SDL keysyms to PRU bits to create key events when PRU bits set */
static SDLKey BESPRUMap[PLAYER_TOTAL][PRU_BITS] = {
  /* SNES gamepad protocol button order:
     ("Bx" is "button x", Dx is "d-pad x", and SE/ST are select/start)
     BB, BY, SE, ST, DU, DD, DL, DR, BA, BX, BL, BR */
  /* SNES Gamepad 0 */
  { BES_P1_BB, BES_P1_BY, BES_P1_SE, BES_P1_ST, BES_P1_DU, BES_P1_DD,
    BES_P1_DL, BES_P1_DR, BES_P1_BA, BES_P1_BX, BES_P1_BL, BES_P1_BR },
  /* SNES Gamepad 1 */
  { BES_P2_BB, BES_P2_BY, BES_P2_SE, BES_P2_ST, BES_P2_DU, BES_P2_DD,
    BES_P2_DL, BES_P2_DR, BES_P2_BA, BES_P2_BX, BES_P2_BL, BES_P2_BR }
};

/* Because things are NEVER easy, remap the button order of the SNES 
  controller protocol via the PRU to the order used in the rest of BES 
  to handle the pause combo. */
//static int8_t pruRemap[PRU_BITS] = { 3, 5, 6, 7, -1, -1, -1, -1, 2, 4, 0, 1 };
static int8_t pruRemap[(PRU_BITS * 2)] = { 3, -1, /* B */ 
  5, -1,  /* Y */
  6, -1,  /* Select */ 
  7, -1,  /* Start */
  -1, -1, -1, -1, -1, -1, -1, -1, /* D-pad */
  2, -1,  /* A */
  4, -1,  /* X */
  0, -1,  /* L */
  1, -1   /* R */
};

/* Functions in pru.cpp (TI-licensed code, so it can't go in this file) */
extern uint32_t BESPRUSetup(void);
extern void BESPRUShutdown(void);
extern uint32_t BESPRUCheckState(void);
/* ---- END: PRU gamepad constants/variables/functions ---- */

/* ---- START: GPIO gamepad constants/variables/functions ---- */
#define GPIO_DATA_IN_REG	0x138
#define GPIO_DATA_OUT_REG	0x13C
#define GPIO_SETDATAOUT		0x194
#define GPIO_CLEARDATAOUT	0x190
#define NUM_GPIO_BANKS 4
#define BUF_SIZE 128
static const uint32_t gpioAddrs[NUM_GPIO_BANKS] =
  {0x44E07000, 0x4804C000, 0x481AC000, 0x481AE000 };
static const uint32_t controlAddr = 0x44E10000;
static uint32_t *gpios[NUM_GPIO_BANKS];
static uint32_t *controlModule;
static uint8_t currentGPIOState[GPIO_TOTAL];
static uint8_t lastGPIOState[GPIO_TOTAL];
static uint8_t changeGPIOState[GPIO_TOTAL];
static uint32_t gpioEnabled = 0;

/* Mapping of GPIO signals to our "logical" gamepads (pulled from db) */
uint8_t BESGPIOMap[GPIO_TOTAL] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static void processGPIOInput(void);
/* ---- END: GPIO constants/variables/functions ---- */



/* Bitmask of buttons that trigger a pause */
uint32_t BESPauseCombo = 0;
/* Current bitmask of buttons being pressed */
uint32_t BESPauseComboCurrent = 0;

/* Mapping of gamepad button numbers to our "logical" gamepads. The 
  first BUTTON_TOTAL entries are buttons, the next AXIS_TOTAL entries
  are the axis numbers for the d-pad. */
uint8_t BESJoystickButtonMap[PLAYER_TOTAL][BUTTON_TOTAL + AXIS_TOTAL] = {
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
/* Mapping of gamepad axis settings (invert and deadzone) */
uint8_t BESJoystickAxisSettingMap[PLAYER_TOTAL][AXIS_TOTAL][AXISSETTING_TOTAL];

/* When the d-pad comes back to center, we have to do a keyup event for
  the previous direction to let the emus know the d-pad is released. */
static SDLKey gamepadAxisLastActive[PLAYER_TOTAL][AXIS_TOTAL] = {
    /* Gamepad 0 */
    {SDLK_UNKNOWN, SDLK_UNKNOWN},
    /* Gamepad 1 */
    {SDLK_UNKNOWN, SDLK_UNKNOWN}
};

/* Mapping of gamepad d-pad to U/D/L/R keypresses */
static SDLKey gamepadAxisKeyMap[PLAYER_TOTAL][AXIS_TOTAL][2] = {
  /* Gamepad 0 */
  {
    /* Axis 0 (left/right) */
    {BES_P1_DL, BES_P1_DR},
    /* Axis 1 (up/down) */
    {BES_P1_DU, BES_P1_DD}
  },
  /* Gamepad 1 */
  {
    /* Axis 0 (left/right) */
    {BES_P2_DL, BES_P2_DR},
    /* Axis 1 (up/down) */
    {BES_P2_DU, BES_P2_DD},
  }
};

/* Gamepad maps to keys: L, R, A, B, X, Y, Start, Select, Pause */
static SDLKey BESKeyMap[PLAYER_TOTAL][BUTTON_TOTAL] = {
  {BES_P1_BL, BES_P1_BR, BES_P1_BA, BES_P1_BB,
#if !defined(BUILD_SNES)
    SDLK_UNKNOWN, SDLK_UNKNOWN,
#else
    BES_P1_BX, BES_P1_BY,
#endif /* BUILD_SNES */
    BES_P1_ST, BES_P1_SE, BES_PAUSE},
  {BES_P2_BL, BES_P2_BR, BES_P2_BA, BES_P2_BB, BES_P2_BX,
    BES_P2_BY, BES_P2_ST, BES_P2_SE, BES_PAUSE}
};

#if defined(BEAGLEBONE_BLACK)
static const char *joystickPath[PLAYER_TOTAL*2] = {
  /* USB device paths for gamepads plugged into the USB host port */
  "/dev/input/by-path/platform-musb-hdrc.1.auto-usb-0:1:1.0-joystick",
  "DUMMY",
  /* USB device paths for gamepads plugged into a USB hub */
  "/dev/input/by-path/platform-musb-hdrc.1.auto-usb-0:1.1:1.0-joystick",
  "/dev/input/by-path/platform-musb-hdrc.1.auto-usb-0:1.2:1.0-joystick" };
#else
static const char *joystickPath[PLAYER_TOTAL] = {
  "/dev/input/by-path/pci-0000:02:00.0-usb-0:2.1:1.0-joystick",
  "/dev/input/by-path/pci-0000:02:00.0-usb-0:2.2:1.0-joystick" };
#endif
static SDL_Joystick *joystick[PLAYER_TOTAL] = {NULL, NULL};
static int32_t BESDeviceMap[PLAYER_TOTAL] = {-1, -1};

/* Is a USB gamepad plugged in? (1 if yes, 0 if no) */
#define JOYSTICK_PLUGGED 1
#define JOYSTICK_UNPLUGGED 0
uint32_t BESJoystickPresent[PLAYER_TOTAL] = {0, 0};

/* Bitmask of which joysticks are plugged in (1) and which aren't (0) */
static int joystickState = 0;

void BESResetJoysticks(void) {
  joystickState = 0;
  BESCheckJoysticks();
}

void BESCheckJoysticks(void) {
  bool update = false;
  bool restartJoysticks = false;
  int oldJoystickState = joystickState;
  joystickState = 0;
  char tempBuf[128];
  int retVal = 0, i = 0;

  /* Check if the joysticks currently exist */
#if defined(BEAGLEBONE_BLACK)
  for (i=0; i < (PLAYER_TOTAL*2); i++) {
#else
  for (i=0; i < PLAYER_TOTAL; i++) {
#endif
    update = false;
    retVal = readlink(joystickPath[i], tempBuf, sizeof(tempBuf)-1);

    if ((retVal == -1) /* No joystick */ &&
      ((oldJoystickState >> i) & 0x1) /* Joystick was plugged in */ )
    {
      update = true;
      restartJoysticks = true;
      fprintf(stderr, "Player %d Joystick has been unplugged\n", i+1);
    }
    else if ((retVal != -1) /* Joystick exists */ &&
      !((oldJoystickState >> i) & 0x1) /* Joystick was not plugged in */ )
    {
      joystickState |= (JOYSTICK_PLUGGED << i); /* Joystick is now plugged in */
      update = true;
      restartJoysticks = true;
      fprintf(stderr, "Player %d Joystick has been plugged in\n", i+1);
    }
    else
      joystickState |= ((oldJoystickState >> i) & 0x1) << i;

    if (update) {
#if defined(BEAGLEBONE_BLACK)
    if (retVal != -1) {
      /* Are we updating the gamepad in the USB host port? */
      if (i < PLAYER_TOTAL) {
        BESDeviceMap[tempBuf[retVal - 1] - '0'] = i;
        BESJoystickPresent[i] = 1;
      /* Are we updating gamepads plugged into a USB hub? */
      } else {
        BESDeviceMap[tempBuf[retVal - 1] - '0'] = (i-PLAYER_TOTAL);
        BESJoystickPresent[(i-PLAYER_TOTAL)] = 1;
      }
    }
    else
    {
      /* Are we updating the gamepad in the USB host port? */
      if (i < PLAYER_TOTAL) {
        BESDeviceMap[i] = -1;
        BESJoystickPresent[i] = 0;
      /* Are we updating gamepads plugged into a USB hub? */
      } else {
        BESDeviceMap[(i-PLAYER_TOTAL)] = -1;
        BESJoystickPresent[(i-PLAYER_TOTAL)] = 0;
      }
    }
#else
    if (retVal != -1) {
      BESDeviceMap[tempBuf[retVal - 1] - '0'] = i;
      BESJoystickPresent[i] = 1;
    }
    else {
      BESDeviceMap[i] = -1;
      BESJoystickPresent[i] = 0;
    }
#endif
    } /* End "update" check */
  }

  /* Restart the joystick subsystem */
  if (restartJoysticks) {
    /* Shut down the joystick subsystem */
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);

    /* Turn the joystick system back on if we have at least one joystick */
    if (joystickState) {
      SDL_InitSubSystem(SDL_INIT_JOYSTICK);
      for (i=0; i < PLAYER_TOTAL; i++) {
        joystick[i] = SDL_JoystickOpen(i);
      }
      SDL_JoystickEventState(SDL_ENABLE);
    }
  }
}

/* Turn gamepad events into keyboard events */
void BESProcessJoystickEvent(const SDL_Event *event)
{
  SDL_Event keyEvent;
  int button = 0, axis = 0, gamepad = 0;

  switch(event->type) {
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
      if (event->type == SDL_JOYBUTTONDOWN) keyEvent.type = SDL_KEYDOWN;
      else keyEvent.type = SDL_KEYUP;
      keyEvent.key.keysym.sym = SDLK_UNKNOWN;
      keyEvent.key.keysym.mod = KMOD_NONE;
      for (gamepad = 0; gamepad < PLAYER_TOTAL; gamepad++)
      {
        if (BESDeviceMap[event->jbutton.which] == gamepad)
        {
          for (button = 0; button < BUTTON_TOTAL; button++)
          {
            if (event->jbutton.button == BESJoystickButtonMap[gamepad][button])
            {
              keyEvent.key.keysym.sym = BESKeyMap[gamepad][button];
              if ( !gamepad && BESPauseCombo ) /* Player 1 */
              {
                if (keyEvent.type == SDL_KEYDOWN)
                  BESPauseComboCurrent |= (1 << /*event->jbutton.*/button);
                else
                  BESPauseComboCurrent &= (1 << /*event->jbutton.*/button);
                if ((BESPauseComboCurrent & BESPauseCombo) == BESPauseCombo)
                {
                  keyEvent.key.keysym.sym = BES_PAUSE;
                  keyEvent.type = SDL_KEYDOWN;
                  BESPauseComboCurrent = 0;
                }
              }
              if (keyEvent.type == SDL_KEYDOWN)
                keyEvent.key.state = SDL_PRESSED;
              SDL_PushEvent(&keyEvent);
              return;
            }
          } /* End switch */
        } /* End if */
      } /* End for loop */
      break;

    case SDL_JOYAXISMOTION:
      //fprintf(stderr, "SDL_JOYSAXISMOTION for device %d\n", event->jaxis.which);
      for (gamepad = 0; gamepad < PLAYER_TOTAL; gamepad++)
      {
        if (BESDeviceMap[event->jaxis.which] == gamepad)
        {
          for (axis = 0; axis < AXIS_TOTAL; axis++)
          {
            if (BESJoystickButtonMap[gamepad][axis+BUTTON_TOTAL] == event->jaxis.axis)
            {
              if (event->jaxis.value < 0) {
                keyEvent.key.keysym.sym =
                  gamepadAxisKeyMap[gamepad][axis][0];
                keyEvent.type = SDL_KEYDOWN;
                keyEvent.key.state = SDL_PRESSED;
                gamepadAxisLastActive[gamepad][axis] =
                  gamepadAxisKeyMap[gamepad][axis][0];
              } else if (event->jaxis.value > 0) {
                keyEvent.key.keysym.sym =
                  gamepadAxisKeyMap[gamepad][axis][1];
                keyEvent.type = SDL_KEYDOWN;
                keyEvent.key.state = SDL_PRESSED;
                gamepadAxisLastActive[gamepad][axis] =
                  gamepadAxisKeyMap[gamepad][axis][1];
              }
              else {
                keyEvent.key.keysym.sym =
                  gamepadAxisLastActive[gamepad][axis];
                keyEvent.type = SDL_KEYUP;
              }
              SDL_PushEvent(&keyEvent);
              return;
            } /* End buttonmap if */
          } /* End axis for loop */
        } /* End gamepad if */
      } /* End gamepad for */
      break;
  }
}

static void processPRUInput(void)
{
  uint32_t xorDiff;
  uint32_t rawPRUData, bit;
  uint32_t controller, i;

  /* Change in the PRU state since the last sample */
  rawPRUData = BESPRUCheckState();
#if 0
  /* Is controller 0 plugged in? */
  if (rawPRUData & 0xA) /* Bit pattern 1010 */
    BESPRUGamepadPresent[0] = 1;
  else 
    BESPRUGamepadPresent[0] = 0;

  /* Is controller 1 plugged in? */
  if (rawPRUData & 0x5) /* Bit pattern 0101 */
    BESPRUGamepadPresent[1] = 1;
  else
    BESPRUGamepadPresent[1] = 0;
#endif
  if (lastPRUSample != rawPRUData) {
    xorDiff = lastPRUSample ^ rawPRUData;
    lastPRUSample = rawPRUData;

    for (controller=0; controller < PLAYER_TOTAL; controller++) {
      SDL_Event event;
      event.key.keysym.mod = KMOD_NONE;

      if (BESPRUGamepadPresent[controller])
      {
      for (i = 0; i < PRU_BITS; i++) {
        event.key.state = SDL_RELEASED;
        /* Isolate the bit */
        bit = 31 - ((i*2) + controller);

        /* Did this bit change? */
        if (xorDiff & (1 << bit))
        {
          /* Determine what key it maps to */
          event.key.keysym.sym = BESPRUMap[controller][i];
          /* Determine if it is a keyup or keydown */
          if (lastPRUSample & (1 << bit)) {
            event.type = SDL_KEYUP;
            if (pruRemap[/*bit*/((i*2) + controller)] != -1) {
              BESPauseComboCurrent &= ~(1 << pruRemap[/*bit*/((i*2) + controller)]);
            }
          } 
          else {
            event.type = SDL_KEYDOWN;
            event.key.state = SDL_PRESSED;
            if (pruRemap[/*bit*/((i*2) + controller)] != -1) {
              BESPauseComboCurrent |= (1 << pruRemap[/*bit*/((i*2) + controller)]);
            }
          }
          
          /* Did we trigger the pause combo? */
          if ((BESPauseComboCurrent & BESPauseCombo) == BESPauseCombo)
          {
            event.key.keysym.sym = BES_PAUSE;
            event.type = SDL_KEYDOWN;
            BESPauseComboCurrent = 0;
          }

          /* Insert the newly-created key event */
          SDL_PushEvent(&event);
        } /* end if */
      } /* end for loop */

      } /* end if controller present */
    } /* end for loop */
  } /* end if */

}

uint32_t BESGPIOSetup(void)
{
  int fd;
  char buf[BUF_SIZE];
  int i;

  memset(currentGPIOState, 0, sizeof(uint8_t) * GPIO_TOTAL);
  memset(lastGPIOState, 0, sizeof(uint8_t) * GPIO_TOTAL);
  memset(changeGPIOState, 0, sizeof(uint8_t) * GPIO_TOTAL);
  gpioEnabled = 1;

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (fd < 0)
  {
    fprintf(stderr, "Unable to set up GPIOs, skipping...\n");
    gpioEnabled = 0;
    return 1;
  }
  close(fd);

  i = chmod("/sys/class/gpio/export", 0666);
  if (i < 0)
  {
    fprintf(stderr, "Unable to chmod GPIO export, skipping...\n");
    gpioEnabled = 0;
    return 1;
  }

  /* Export and init all of the GPIO pins */
  for (i = 0; i < GPIO_TOTAL; i++)
  {
    if (BESGPIOMap[i] < 0xFF)
    {
      snprintf(buf, (BUF_SIZE - 1), "%d", BESGPIOMap[i]);
      fd = open("/sys/class/gpio/export", O_WRONLY);
      if (fd < 0)
      {
        fprintf(stderr, "Could not open /sys/class/gpio/export\n");
        gpioEnabled = 0;				
      }

      if (write(fd, buf, strlen(buf)) < 0)
      {
        fprintf(stderr, "Could not export pin %d\n", BESGPIOMap[i]);
      }
      close(fd);

      /* Set up this pin as an input */
      snprintf(buf, (BUF_SIZE - 1), "/sys/class/gpio/gpio%d/direction", BESGPIOMap[i]);
      fd = open(buf, O_WRONLY);
      write(fd, "in", 2);
      close(fd);
    } /* End if */
  } /* End for */

  /* If the pins were exported, mmap the GPIO registers */
  if (gpioEnabled)
  {
    /* Access /dev/mem */
    fd = open("/dev/mem", O_RDWR | O_SYNC | O_CLOEXEC);
    if (fd < 0)
    {
      fprintf(stderr, "Unable to mmap() GPIOs, disabling GPIO support\n");
      gpioEnabled = 0;
      return(gpioEnabled);
    }

    /* mmap() the controlModule */
    controlModule = (uint32_t *) mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, controlAddr);
    fprintf(stderr, "Control module at address 0x%08x mapped\n", controlAddr);

    /* mmap() the four GPIO banks */
    for (i = 0; i < 4; i++)
    {
      gpios[i] = (uint32_t *) mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, gpioAddrs[i]);
      fprintf(stderr, "gpio[%i] at address 0x%08x mapped\n",i,gpioAddrs[i]);
    }	
    close(fd);
  }

  /* Get the initial GPIO states */
  processGPIOInput();
  processGPIOInput();

  /* Done! */
  return(gpioEnabled);
}

void BESGPIOToggle(const int gpio, const int value)
{
  uint32_t reg, mask;
  uint8_t bank;

  if (!gpioEnabled) return;

  bank = gpio / 32;
  mask = 1 << (gpio % 32);
  reg = gpios[bank][GPIO_DATA_OUT_REG / 4];
  if (value)
    gpios[bank][GPIO_DATA_OUT_REG/4] |= mask;
  else
    gpios[bank][GPIO_DATA_OUT_REG/4] = (reg & (0xFFFFFFFF - mask));
}
 
static void processGPIOInput(void)
{
  SDL_Event event;
  int i;
  uint32_t reg, mask;
  uint8_t bank;

  /* Make our "current" GPIO states the "last" states */
  memcpy(lastGPIOState, currentGPIOState, sizeof(uint8_t) * GPIO_TOTAL);

  /* Sweep the GPIOs and see what has changed */
  for (i = 0; i < GPIO_TOTAL; i++)
  {
    if (BESGPIOMap[i] != 0xFF)
    {
      bank = BESGPIOMap[i] / 32;
      mask = 1 << (BESGPIOMap[i] % 32);
      reg = gpios[bank][GPIO_DATA_IN_REG / 4];
      currentGPIOState[i] = ((reg & mask) >> (BESGPIOMap[i] % 32));
      if (currentGPIOState[i] == lastGPIOState[i])
        changeGPIOState[i] = 0;
      else
        changeGPIOState[i] = 1;
    } else
      changeGPIOState[i] = 0;
  }

  /* Generate keyboard events if needed */
  event.key.keysym.mod = KMOD_NONE;
  for (i=0; i < GPIO_TOTAL; i++)
  {
    if (changeGPIOState[i])
    {
      if (currentGPIOState[i])
        event.type = SDL_KEYDOWN;
      else
        event.type = SDL_KEYUP;

      switch (i) 
      {
        case GPIO_GPLEFT:
          event.key.keysym.sym = BES_P1_DL; break;
        case GPIO_GPRIGHT:
          event.key.keysym.sym = BES_P1_DR; break;
        case GPIO_GPUP:
          event.key.keysym.sym = BES_P1_DU; break;
        case GPIO_GPDOWN:
          event.key.keysym.sym = BES_P1_DD; break;
        case GPIO_LEFT:
          event.key.keysym.sym = BES_P1_BL; break;
        case GPIO_RIGHT:
          event.key.keysym.sym = BES_P1_BR; break;
        case GPIO_A:
          event.key.keysym.sym = BES_P1_BA; break;
        case GPIO_B:
          event.key.keysym.sym = BES_P1_BB; break;
        case GPIO_X:
          event.key.keysym.sym = BES_P1_BX; break;
        case GPIO_Y:
          event.key.keysym.sym = BES_P1_BY; break;
        case GPIO_START:
          event.key.keysym.sym = BES_P1_ST; break;
        case GPIO_SELECT:
          event.key.keysym.sym = BES_P1_SE; break;
        case GPIO_PAUSE:
          event.key.keysym.sym = BES_PAUSE; break;
        default:
          break;
      } /* end switch */
			
      /* Filter out pause menu button up event */
      if (!((i == 12) && (event.type == SDL_KEYUP)))
        /* Insert event into event queue */
        SDL_PushEvent(&event);

    } /* end changeState */	
  } /* end for loop */
}

void BESProcessEvents(void)
{
  if (gpioEnabled)
    processGPIOInput();

  if (pruEnabled)
    processPRUInput();

}

void BESControlSetup(void)
{
  BESGPIOSetup();
  pruEnabled = BESPRUSetup();
  if (!pruEnabled) {
    BESPRUGamepadPresent[0] = 0;
    BESPRUGamepadPresent[1] = 0;
  }
}

void BESControlShutdown(void)
{
  /* AWH - No need to shut down GPIOs... */
  BESPRUShutdown();
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL.h>

#include "gui.h"

/* Bitmask of buttons that trigger a pause */
uint32_t BESPauseCombo = 0;
/* Current bitmask of buttons being pressed */
uint32_t BESPauseComboCurrent = 0;
/* Mapping of gamepad button numbers to our "logical" gamepads. The 
  first BUTTON_TOTAL entries are buttons, the next AXIS_TOTAL entries
  are the axis numbers for the d-pad. */
uint8_t BESButtonMap[NUM_JOYSTICKS][BUTTON_TOTAL + AXIS_TOTAL] = {
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
/* Mapping of gamepad axis settings (invert and deadzone) */
uint8_t BESAxisMap[NUM_JOYSTICKS][AXIS_TOTAL][AXISSETTING_TOTAL];

/* Mapping of GPIO signals to our "logical" gamepads */
uint8_t GPIOButtonMap[GPIO_TOTAL] = { 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

/* When the d-pad comes back to center, we have to do a keyup event for
  the previous direction to let the emus know the d-pad is released. */
static SDLKey gamepadAxisLastActive[NUM_JOYSTICKS][AXIS_TOTAL] = {
    /* Gamepad 0 */
    {SDLK_UNKNOWN, SDLK_UNKNOWN},
    /* Gamepad 1 */
    {SDLK_UNKNOWN, SDLK_UNKNOWN}
};

/* Mapping of gamepad d-pad to U/D/L/R keypresses */
static SDLKey gamepadAxisKeyMap[NUM_JOYSTICKS][AXIS_TOTAL][2] = {
  /* Gamepad 0 */
  {
    /* Axis 0 (left/right) */
    {SDLK_LEFT, SDLK_RIGHT},
    /* Axis 1 (up/down) */
    {SDLK_UP, SDLK_DOWN}
  },
  /* Gamepad 1 */
  {
    /* Axis 0 (left/right) */
    {SDLK_v, SDLK_h},
    /* Axis 1 (up/down) */
    {SDLK_g, SDLK_b},
  }
};

/* Gamepad maps to keys: L, R, A, B, X, Y, Select, Start, Pause */
static SDLKey gamepadButtonKeyMap[NUM_JOYSTICKS][BUTTON_TOTAL] = {
  {SDLK_a, SDLK_s, SDLK_z, SDLK_x,
#if !defined(BUILD_SNES)
    SDLK_UNKNOWN, SDLK_UNKNOWN,
#else
    SDLK_d, SDLK_c,
#endif /* BUILD_SNES */
    SDLK_RETURN, SDLK_BACKSPACE, SDLK_n},
  {SDLK_j, SDLK_k, SDLK_m, SDLK_COMMA, SDLK_l,
    SDLK_SEMICOLON, SDLK_SLASH, SDLK_PERIOD, SDLK_n}
};

#if defined(BEAGLEBONE_BLACK)
static const char *joystickPath[NUM_JOYSTICKS*2] = {
  /* USB device paths for gamepads plugged into the USB host port */
  "/dev/input/by-path/platform-musb-hdrc.1.auto-usb-0:1:1.0-joystick",
  "DUMMY",
  /* USB device paths for gamepads plugged into a USB hub */
  "/dev/input/by-path/platform-musb-hdrc.1.auto-usb-0:1.1:1.0-joystick",
  "/dev/input/by-path/platform-musb-hdrc.1.auto-usb-0:1.2:1.0-joystick" };
#else
static const char *joystickPath[NUM_JOYSTICKS] = {
  "/dev/input/by-path/pci-0000:02:00.0-usb-0:2.1:1.0-joystick",
  "/dev/input/by-path/pci-0000:02:00.0-usb-0:2.2:1.0-joystick" };
#endif
static SDL_Joystick *joystick[NUM_JOYSTICKS] = {NULL, NULL};
static int32_t BESDeviceMap[NUM_JOYSTICKS] = {-1, -1};

/* Is a USB gamepad plugged in? (1 if yes, 0 if no) */
#define JOYSTICK_PLUGGED 1
#define JOYSTICK_UNPLUGGED 0
uint32_t BESControllerPresent[NUM_JOYSTICKS] = {0, 0};

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
  for (i=0; i < (NUM_JOYSTICKS*2); i++) {
#else
  for (i=0; i < NUM_JOYSTICKS; i++) {
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
      if (i < NUM_JOYSTICKS) {
        BESDeviceMap[tempBuf[retVal - 1] - '0'] = i;
        BESControllerPresent[i] = 1;
      /* Are we updating gamepads plugged into a USB hub? */
      } else {
        BESDeviceMap[tempBuf[retVal - 1] - '0'] = (i-NUM_JOYSTICKS);
        BESControllerPresent[(i-NUM_JOYSTICKS)] = 1;
      }
    }
    else
    {
      /* Are we updating the gamepad in the USB host port? */
      if (i < NUM_JOYSTICKS) {
        BESDeviceMap[i] = -1;
        BESControllerPresent[i] = 0;
      /* Are we updating gamepads plugged into a USB hub? */
      } else {
        BESDeviceMap[(i-NUM_JOYSTICKS)] = -1;
        BESControllerPresent[(i-NUM_JOYSTICKS)] = 0;
      }
    }
#else
    if (retVal != -1) {
      BESDeviceMap[tempBuf[retVal - 1] - '0'] = i;
      BESControllerPresent[i] = 1;
    }
    else {
      BESDeviceMap[i] = -1;
      BESControllerPresent[i] = 0;
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
      for (i=0; i < NUM_JOYSTICKS; i++) {
        joystick[i] = SDL_JoystickOpen(i);
      }
      SDL_JoystickEventState(SDL_ENABLE);
    }
  }
}

/* Turn gamepad events into keyboard events */
void handleJoystickEvent(const SDL_Event *event)
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
      for (gamepad = 0; gamepad < NUM_JOYSTICKS; gamepad++)
      {
        if (BESDeviceMap[event->jbutton.which] == gamepad)
        {
          for (button = 0; button < BUTTON_TOTAL; button++)
          {
            if (event->jbutton.button == BESButtonMap[gamepad][button])
            {
              keyEvent.key.keysym.sym =
                gamepadButtonKeyMap[gamepad][button];
              if ( !gamepad && BESPauseCombo ) /* Player 1 */
              {
                if (keyEvent.type == SDL_KEYDOWN)
                  BESPauseComboCurrent |= (1 << /*event->jbutton.*/button);
                else
                  BESPauseComboCurrent &= (1 << /*event->jbutton.*/button);
                if ((BESPauseComboCurrent & BESPauseCombo) == BESPauseCombo)
                {
                  keyEvent.key.keysym.sym = SDLK_n;
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
            if (BESButtonMap[gamepad][axis+BUTTON_TOTAL] == event->jaxis.axis)
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


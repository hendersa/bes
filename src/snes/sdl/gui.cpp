#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_mixer.h>

#include "gui.h"

#if defined(BEAGLEBONE_BLACK)
// USB hotplugging work around hack
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#endif

/* Dialog info */
TTF_Font *fontFSB12, *fontFSB16, *fontFSB25;
TTF_Font *fontFS16, *fontFS20, *fontFS24, *fontFS25, *fontFS30;
SDL_Surface *dlgMarker;
SDL_Color textColor={255,255,255,255};
SDL_Color inactiveTextColor={96,96,96,255};
SDL_Rect guiPiece[TOTAL_PIECES] = {
        {0,0,8,8}, {24,0,8,8}, {64,0,8,8}, {88,0,8,8},
        {8,0,8,4}, {72,4,8,4}, {32,0,4,8}, {44,0,4,8}
};
guiSize_t guiSize;

/* Gamepad maps to keys: L, R, A, B, X, Y, Select, Start, Pause */
static SDLKey gamepadButtonKeyMap[2][9] = {
  {SDLK_a, SDLK_s, SDLK_z, SDLK_x, SDLK_d,
    SDLK_c, SDLK_BACKSPACE, SDLK_RETURN, SDLK_n},
  {SDLK_j, SDLK_k, SDLK_m, SDLK_COMMA, SDLK_l,
    SDLK_SEMICOLON, SDLK_PERIOD, SDLK_SLASH, SDLK_n}
};
static SDLKey gamepadAxisKeyMap[2][2][2] = {
  /* Gamepad 0 */
  {
    /* Axis 0 (up/down) */
    {SDLK_UP, SDLK_DOWN},
    /* Axis 1 (left/right) */
    {SDLK_LEFT, SDLK_RIGHT}
  },
  /* Gamepad 1 */
  {
    /* Axis 0 (up/down) */
    {SDLK_g, SDLK_b},
    /* Axis 1 (left/right) */
    {SDLK_v, SDLK_h},
  }
};

static SDLKey gamepadAxisLastActive[2][2] = {
    /* Gamepad 0 */
    {SDLK_UNKNOWN, SDLK_UNKNOWN},
    /* Gamepad 1 */
    {SDLK_UNKNOWN, SDLK_UNKNOWN}
};

static int i, retVal, done;
uint32_t BESPauseComboCurrent = 0;

SDL_Surface *screen256, *screen512, *screenPause;
static SDL_Surface *screen;
SDL_Surface *fbscreen;

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
int BESDeviceMap[NUM_JOYSTICKS] = {-1, -1};
int BESControllerPresent[NUM_JOYSTICKS] = {0, 0};
 

#define JOYSTICK_PLUGGED 1
#define JOYSTICK_UNPLUGGED 0

/* Bitmask of which joysticks are plugged in (1) and which aren't (0) */
static int joystickState = 0;

//extern gameInfo_t *gameInfo;
int totalGames;
int audioAvailable = 1;
int currentVolume = 64; // AWH32;
int volumeOverlayCount;

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
void handleJoystickEvent(SDL_Event *event)
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
      for (gamepad = 0; gamepad < 2; gamepad++)
      {
        if (BESDeviceMap[event->jbutton.which] == gamepad)
        {
          for (button = 0; button < (TAG_LAST_CONTROL - TAG_FIRST_BUTTON); button++)
          {
            if (event->jbutton.button == BESButtonMap[gamepad][button + (TAG_FIRST_BUTTON - TAG_FIRST_CONTROL)])
            {
              keyEvent.key.keysym.sym = 
                gamepadButtonKeyMap[gamepad][button];
              if ( !gamepad && BESPauseCombo ) /* Player 1 */
              {
                if (keyEvent.type == SDL_KEYDOWN)
                  BESPauseComboCurrent |= (1 << event->jbutton.button);
                else
                  BESPauseComboCurrent &= (1 << event->jbutton.button);
//fprintf(stderr, "BESPauseComboCurrent: %d BESPauseCombo: %d\n",
//  BESPauseComboCurrent, BESPauseCombo);
                if ((BESPauseComboCurrent & BESPauseCombo) == BESPauseCombo)
                {
                  keyEvent.key.keysym.sym = SDLK_n;
                  keyEvent.type = SDL_KEYDOWN;
                  BESPauseComboCurrent = 0;
                }
              } 
              SDL_PushEvent(&keyEvent);
              return;
            }
          } /* End switch */
        } /* End if */
      } /* End for loop */
      break;

    case SDL_JOYAXISMOTION:
      for (gamepad = 0; gamepad < 2; gamepad++)
      {
        if (BESDeviceMap[event->jaxis.which] == gamepad)
        {
          for (axis = 0; axis < 2; axis++)
          {
            if (BESButtonMap[gamepad][axis] == event->jaxis.axis) 
            {
              if (event->jaxis.value < 0) {
                keyEvent.key.keysym.sym = 
                  gamepadAxisKeyMap[gamepad][axis][0];
                keyEvent.type = SDL_KEYDOWN;
                gamepadAxisLastActive[gamepad][axis] = 
                  gamepadAxisKeyMap[gamepad][axis][0];
              } else if (event->jaxis.value > 0) {
                keyEvent.key.keysym.sym = 
                  gamepadAxisKeyMap[gamepad][axis][1];
                keyEvent.type = SDL_KEYDOWN;
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

static void quit(int rc)
{
  TTF_Quit();
  SDL_Quit();
  exit(rc);
}

int doGuiSetup(void)
{
  /* Initialize SDL */
  if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0 ) {
    fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
    return(1);
  }

  if ( SDL_Init(SDL_INIT_AUDIO) < 0 ) {
    fprintf(stderr, "Couldn't initialize audio: %s\n",SDL_GetError());
    audioAvailable = 0;
  }

  /* Set video mode */
#if defined(PC_PLATFORM)
  if ( (fbscreen = SDL_SetVideoMode(/*720, 480,*/320,240, 16, 0)) == NULL ) {
    fprintf(stderr, "Couldn't set 720x480x16 video mode: %s\n",
      SDL_GetError());
    quit(2);
  }
#else
  if ( (fbscreen = SDL_SetVideoMode(0, 0, 16, 0)) == NULL) {
    fprintf(stderr, "Couldn't set native 16 BPP video mode: %s\n",
      SDL_GetError());
    quit(2);
  }
#endif /* PC_PLATFORM */
  screenPause = SDL_CreateRGBSurface(0, 512, 512,
    16, fbscreen->format->Rmask, fbscreen->format->Gmask,
    fbscreen->format->Bmask, fbscreen->format->Amask);
  screen512 = SDL_CreateRGBSurface(0, 512, 512,
    16, fbscreen->format->Rmask, fbscreen->format->Gmask,
    fbscreen->format->Bmask, fbscreen->format->Amask);
  screen256 = SDL_CreateRGBSurface(0, 256, 256,
    16, fbscreen->format->Rmask, fbscreen->format->Gmask,
    fbscreen->format->Bmask, fbscreen->format->Amask);
  screen = screen512;

  tex256buffer = calloc(1, 256*256*2);
  tex512buffer = calloc(1, 512*512*2);

  /* Shut off the mouse pointer */
  SDL_ShowCursor(SDL_DISABLE);

  /* Turn on EGL */
  EGLInitialize();
  EGLDestSize(fbscreen->w, fbscreen->h);
  EGLSrcSize(512, 512);

  /* Init font engine */
  if(TTF_Init()==-1) {
    fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
    quit(2);
  }

  /* Check for joysticks */
  BESResetJoysticks();

  /* Get SOMETHING on the screen ASAP */
  done = 0;
  SDL_FillRect(screen, NULL, 0x0);
  SDL_UpdateRect(screen, 0, 0, 0, 0);
  EGLBlitGL(screen->pixels);
  EGLFlip();

  gpioEvents();

  dlgMarker = IMG_Load("gfx/pause_marker.png");

  /* Load the fonts for the GUI and dialogs */
  fontFSB12 = TTF_OpenFont("fonts/FreeSansBold.ttf", 12);
  fontFSB16 = TTF_OpenFont("fonts/FreeSansBold.ttf", 16);
  fontFSB25 = TTF_OpenFont("fonts/FreeSansBold.ttf", 25);
  if (!fontFSB12 || !fontFSB16 || !fontFSB25)
    fprintf(stderr, "Unable to load fonts/FreeSansBold.ttf");

  fontFS16 = TTF_OpenFont("fonts/FreeSans.ttf", 16);
  fontFS20 = TTF_OpenFont("fonts/FreeSans.ttf", 20);
  fontFS24 = TTF_OpenFont("fonts/FreeSans.ttf", 24);
  fontFS25 = TTF_OpenFont("fonts/FreeSans.ttf", 25);
  fontFS30 = TTF_OpenFont("fonts/FreeSans.ttf", 30);
  if (!fontFS16 || !fontFS20 || !fontFS24 || !fontFS25 || !fontFS30)
    fprintf(stderr, "Unable to load fonts/FreeSans.ttf");

  return 0;
}


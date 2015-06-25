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

/* Set the resolution at or under which you need to use the
  GUI for "small" screens */
#define GUI_SMALL_WIDTH 480
#define GUI_SMALL_HEIGHT 272

pthread_t loadingThread;
static void *loadingThreadFunc(void *);

#define GRADIENT_Y_POS 55

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
  {SDLK_a, SDLK_s, SDLK_z, SDLK_x, SDLK_UNKNOWN, 
    SDLK_UNKNOWN, SDLK_BACKSPACE, SDLK_RETURN, SDLK_n},
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

int guiQuit;

static int i, retVal, done;
uint32_t BESPauseComboCurrent = 0;

static SDL_Surface *logo;
static SDL_Surface *gradient;
SDL_Surface *screen256, *screen512, *screen1024, *screenPause;
static SDL_Surface *screen;
SDL_Surface *fbscreen;

#if defined(CAPE_LCD3)
static SDL_Rect gradientRect = {0, 24, 0, 0};
static SDL_Rect logoRect = {15, 2, 0, 0};
#else
static SDL_Rect gradientRect = {0, GRADIENT_Y_POS, 0, 0};
static SDL_Rect logoRect = {55, 30, 0, 0};
#endif /* CAPE_LCD3 */

#if defined(CAPE_LCD3)
static SDL_Rect backgroundRect = {0, 55, 320, 240-55};
#else
static SDL_Rect backgroundRect = {0, 80, 720, 480-80};
#endif /* CAPE_LCD3 */

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
static int elapsedTime;
static struct timeval startTime, endTime;
static int menuPressDirection = 0;
static int menuPressDirectionStep = 0;
int volumePressDirection = 0;

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
/* AWH */
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

static void quit(int rc)
{
  TTF_Quit();
  SDL_Quit();
  exit(rc);
}

void shutdownVideo(void) {

  EGLShutdown();
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void reinitVideo(void) {

  /* Initialize SDL */
  if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
    fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
    return;
  }

  /* Set video mode */
#if defined(PC_PLATFORM)
  if ( (fbscreen = SDL_SetVideoMode(/*720, 480,*/320, 240, 16, 0)) == NULL ) {
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
#endif 

  /* Determine the size of the GUI to use */
  if ((fbscreen->w <= GUI_SMALL_WIDTH) || (fbscreen->h <= GUI_SMALL_HEIGHT))
    guiSize = GUI_SMALL;
  else
    guiSize = GUI_NORMAL;

  /* Shut off the mouse pointer */
  SDL_ShowCursor(SDL_DISABLE);

  /* Turn on EGL */
  EGLInitialize();
  EGLDestSize(fbscreen->w, fbscreen->h);
  EGLSrcSizeGui(720, 480, (guiSize == GUI_SMALL) );
}

int doGuiSetup(void)
{
  int i;

  /* Initialize SDL */
  if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0 ) {
    fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
    return(1);
  }

  if (audioAvailable && (SDL_Init(SDL_INIT_AUDIO) < 0)) {
    fprintf(stderr, "Couldn't initialize audio: %s\n",SDL_GetError());
    audioAvailable = 0;
  }

  /* Set video mode */
#if defined(PC_PLATFORM)
  if ( (fbscreen = SDL_SetVideoMode(/*720, 480,*/320, 240, 16, 0)) == NULL ) {
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
  screen1024 = SDL_CreateRGBSurface(0, 1024, 1024, 
    16, fbscreen->format->Rmask, fbscreen->format->Gmask, 
    fbscreen->format->Bmask, fbscreen->format->Amask);
  screen512 = SDL_CreateRGBSurface(0, 512, 512,
    16, fbscreen->format->Rmask, fbscreen->format->Gmask,
    fbscreen->format->Bmask, fbscreen->format->Amask);
  screen256 = SDL_CreateRGBSurface(0, 256, 256,
    16, fbscreen->format->Rmask, fbscreen->format->Gmask,
    fbscreen->format->Bmask, fbscreen->format->Amask);
  screen = screen1024;

  tex256buffer = calloc(1, 256*256*2);
  tex512buffer = calloc(1, 512*512*2);

  /* Shut off the mouse pointer */
  SDL_ShowCursor(SDL_DISABLE);

  /* Turn on EGL */
  EGLInitialize();
  EGLDestSize(fbscreen->w, fbscreen->h);
  /* Splash screen has to run in "normal" GUI size here */
  EGLSrcSize(720, 480);

  /* Init font engine */
  if(TTF_Init()==-1) {
    fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
    quit(2);
  }

  return(0);
}

void disableGuiAudio(void)
{
  if (audioAvailable)
  {
    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
  }
}

void enableGuiAudio(void)
{
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    fprintf(stderr, "Couldn't initialize audio: %s\n",SDL_GetError());
    audioAvailable = 0;
  }
  initAudio();
}

void *loadingThreadFunc(void *)
{
  logo = IMG_Load("gfx/logo_trans.png");
  gradient = IMG_Load("gfx/gradient.png");
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
  
  loadGameConfig();
  loadInstruct();
  loadGameLists();
  loadGameInfo();
  loadAudio();
  loadPauseGui();
  loadGBAGui();

  pthread_exit(0);
}

void doGuiLoadingThread(void)
{
  pthread_create(&loadingThread, NULL, (void* (*)(void*)) &loadingThreadFunc, NULL);
}

int doGui(void) {
  int i, k, r, g, b;
  SDL_Event event;
  Uint16 pixel;

  guiQuit = 0;

  /* Get the screen set up again */
  EGLDestSize(fbscreen->w, fbscreen->h);
  EGLSrcSizeGui(720, 480, (guiSize == GUI_SMALL) );
  SDL_BlitSurface(gradient, NULL, screen, &gradientRect);
  SDL_BlitSurface(logo, NULL, screen, &logoRect);

  /* Check for joysticks */
  BESResetJoysticks();

  /* Get SOMETHING on the screen ASAP */
  done = 0;
  SDL_FillRect(screen, NULL, 0x0);
  SDL_BlitSurface(gradient, NULL, screen, &gradientRect);
  SDL_BlitSurface(logo, NULL, screen, &logoRect);
  SDL_UpdateRect(screen, 0, 0, 0, 0);
  startAudio();

  while ( !done ) {
    gettimeofday(&startTime, NULL);

    renderGameList(screen);
    renderGameInfo(screen, currentSelectedGameIndex());
    renderInstruct(screen, BESControllerPresent[0]);
#ifdef CAPE_LCD3
    renderVolume(screen);
#endif /* CAPE_LCD3 */
    incrementGameListFrame();

    EGLBlitGL(screen->pixels);
    EGLFlip();

    gpioEvents();

    /* Check for events */
    while ( SDL_PollEvent(&event) ) {
      switch (event.type) {
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
        case SDL_JOYAXISMOTION:
          handleJoystickEvent(&event);
          break;

        case SDL_MOUSEBUTTONDOWN:
          break;

        case SDL_KEYUP:
          switch (event.key.keysym.sym) {
            case SDLK_UP:
            case SDLK_DOWN:
            case SDLK_a: /* Left trigger */
            case SDLK_s: /* Right trigger */
              menuPressDirection = 0;
              break;
#if 0 // AWH
            case SDLK_LEFT:
            case SDLK_RIGHT:
              volumePressDirection = 0;
              break;
#endif // AWH
            default:
              break;
          }
          break;

        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
#if 0 // AWH
            case SDLK_LEFT:
              shiftSelectedVolumeUp();
              break;
            case SDLK_RIGHT:
              shiftSelectedVolumeDown();
              break;
#endif // AWH
            /* Gamepad left trigger */
            case SDLK_a:
              menuPressDirection = -1;
              menuPressDirectionStep = 6;
              break;
            /* Gamepad right trigger */
            case SDLK_s:
              menuPressDirection = 1;
              menuPressDirectionStep = 6;
              break;
            /* Gamepad up */
            case SDLK_UP:
              menuPressDirection = -1;
              menuPressDirectionStep = 1;
              break;
            /* Gamepad down */
            case SDLK_DOWN:
              menuPressDirection = 1;
              menuPressDirectionStep = 1;
              break;
            /* Gamepad start */
            case SDLK_RETURN:
            /* Gamepad select */
            case SDLK_BACKSPACE:
              if (acceptButton()) {
                playSelectSnd();
                done = 1;
              }
              break;
            case SDLK_ESCAPE:
              guiQuit = 1;
              done = 1;
              break;
            case SDLK_n:
              // AWH doPauseGui("TEST.ROM", PLATFORM_SNES);
              break;
            default:
              break;
          } // End keydown switch
          break;
//#endif /* CAPE_LCD3 */
        case SDL_QUIT:
fprintf(stderr, "SDL_QUIT\n");
          guiQuit = 1;
          done = 1;
          break;
        default:
          break;
      } // End eventtype switch
    } // End PollEvent loop
    if (menuPressDirection == -1)
      shiftSelectedGameUp(menuPressDirectionStep);
    else if (menuPressDirection == 1)
      shiftSelectedGameDown(menuPressDirectionStep);

    if (volumePressDirection == 1)
      shiftSelectedVolumeUp();
    else if (volumePressDirection == -1)
      shiftSelectedVolumeDown();

    BESCheckJoysticks();
    gettimeofday(&endTime, NULL);
    elapsedTime = ((endTime.tv_sec - startTime.tv_sec) * 1000000) + 
      (endTime.tv_usec - startTime.tv_usec);
    if (elapsedTime < TIME_PER_FRAME)
      usleep(TIME_PER_FRAME - elapsedTime);
  } // End while loop

  fadeAudio();
  // Fade out
  for (i = 0; i < 16; i++) {
    SDL_LockSurface(screen);
    for (k = 0; k < (screen->w * screen->h); k++) {
      pixel = ((Uint16 *)(screen->pixels))[k];
      r = (((pixel & 0xF800) >> 11) - 2) << 11;
      g = (((pixel & 0x07E0) >> 5) - 4) << 5;
      b = (pixel & 0x001F) - 2;
      if (r < 0) r = 0;
      if (g < 0) g = 0;
      if (b < 0) b = 0;
      ((Uint16 *)(screen->pixels))[k] = r | g | b; 
    }
    SDL_UnlockSurface(screen);
#if 0
#if defined(BEAGLEBONE_BLACK)
#if defined(CAPE_LCD3)
    SDL_UpdateRect(screen, 0, 0, 320, 240);
#else
    SDL_UpdateRect(screen, 0, 0, 720, 480);
#endif
#else
    SDL_UpdateRect(screen, 0, 0, 640, 480);
#endif
#else
    EGLBlitGL(screen->pixels);
    EGLFlip();
#endif
  }

  if (audioAvailable) {
    // This is our cleanup
    usleep(1000000); // Give the audio fadeout a little time
    SDL_CloseAudio();
    /* AWH: These subsystems will be renabled in the Snes9x init code */
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
  }

  SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
  done = 0;
  if (guiQuit) {
    TTF_Quit();
    SDL_Quit();
  }
  return(currentSelectedGameIndex());
}

void shiftSelectedVolumeUp(void) {
  if (currentVolume < 64) {
    currentVolume = currentVolume + 8;
    //fprintf(stderr, "Increase volume to %d\n", currentVolume);
  }
  changeVolume();
}
void shiftSelectedVolumeDown(void) {
  if (currentVolume >= 8) {
    currentVolume = currentVolume - 8;
    //fprintf(stderr, "Decrease volume to %d\n", currentVolume);
  }
  changeVolume();
}
#ifdef CAPE_LCD3
void renderVolume(SDL_Surface *surface) {
  SDL_Rect bar = {45, 170, 20, 30};
  if (!audioAvailable || volumeOverlayCount <= 0) return;

  for (int i = 0; i < (currentVolume >> 3); i++) {
    SDL_FillRect(surface, &bar, 0x07E0);
    bar.x += 30;
  }
  volumeOverlayCount--;
}
#endif /* CAPE_LCD3 */

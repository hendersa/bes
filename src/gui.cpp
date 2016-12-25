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
#include "besKeys.h"
#include "besControls.h"
#include "besCartDisplay.h"

#if defined(BEAGLEBONE_BLACK)
// USB hotplugging work around hack
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#endif

#if !defined(BUILD_SNES)
extern char ipAddress[20];
pthread_t loadingThread;
static void *loadingThreadFunc(void *);

#define GRADIENT_Y_POS 55
#endif /* BUILD_SNES */

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

static int done;

#if !defined(BUILD_SNES)
bool guiQuit;
static SDL_Surface *logo;
static SDL_Surface *gradient;
static SDL_Surface *ipImage;

static SDL_Rect gradientRect = {0, GRADIENT_Y_POS, 0, 0};
static SDL_Rect logoRect = {55, 30, 0, 0};
static SDL_Rect ipRect = {380, 40, 0, 0};

SDL_Surface *screen1024;
#endif /* BUILD_SNES */
SDL_Surface *screen256, *screen512, *screenPause;
static SDL_Surface *screen;
SDL_Surface *fbscreen;

#if !defined (BUILD_SNES)
static int elapsedTime;
static struct timeval startTime, endTime;
static int menuPressDirection = 0;
static int menuPressDirectionStep = 0;
int volumePressDirection = 0;
#endif /* BUILD_SNES */

bool audioAvailable = true;
uint8_t currentVolume = 64; // AWH32;
int volumeOverlayCount;

static void quit(int rc)
{
  TTF_Quit();
  SDL_Quit();
  exit(rc);
}
#if !defined(BUILD_SNES)
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
  if ( (fbscreen = SDL_SetVideoMode(720, 480, 16, 0)) == NULL ) {
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
  EGLSrcSizeGui(720, 480, guiSize);
}
#endif /* BUILD_SNES */

static int loadFonts(void)
{
  dlgMarker = IMG_Load("gfx/pause_marker.png");

  /* Load the fonts for the GUI and dialogs */
  fontFSB12 = TTF_OpenFont("fonts/FreeSansBold.ttf", 12);
  fontFSB16 = TTF_OpenFont("fonts/FreeSansBold.ttf", 16);
  fontFSB25 = TTF_OpenFont("fonts/FreeSansBold.ttf", 25);
  if (!fontFSB12 || !fontFSB16 || !fontFSB25)
  {
    fprintf(stderr, "Unable to load fonts/FreeSansBold.ttf");
    return 1;
  }
  fontFS16 = TTF_OpenFont("fonts/FreeSans.ttf", 16);
  fontFS20 = TTF_OpenFont("fonts/FreeSans.ttf", 20);
  fontFS24 = TTF_OpenFont("fonts/FreeSans.ttf", 24);
  fontFS25 = TTF_OpenFont("fonts/FreeSans.ttf", 25);
  fontFS30 = TTF_OpenFont("fonts/FreeSans.ttf", 30);
  if (!fontFS16 || !fontFS20 || !fontFS24 || !fontFS25 || !fontFS30)
  {
    fprintf(stderr, "Unable to load fonts/FreeSans.ttf");
    return 1;
  }
  return 0;
}

int doGuiSetup(void)
{
  /* Initialize SDL */
  if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0 ) {
    fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
    return(1);
  }

  if (audioAvailable && (SDL_Init(SDL_INIT_AUDIO) < 0)) {
    fprintf(stderr, "Couldn't initialize audio: %s\n",SDL_GetError());
    audioAvailable = 0;
  }
  audioAvailable = 1;

  /* Set video mode */
#if defined(PC_PLATFORM)
  if ( (fbscreen = SDL_SetVideoMode(720, 480, 16, 0)) == NULL ) {
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

  /* Determine the size of the GUI to use */
  if ((fbscreen->w <= GUI_SMALL_WIDTH) || (fbscreen->h <= GUI_SMALL_HEIGHT))
    guiSize = GUI_SMALL;
  else
    guiSize = GUI_NORMAL;

  screenPause = SDL_CreateRGBSurface(0, 512, 512,
    16, fbscreen->format->Rmask, fbscreen->format->Gmask,
    fbscreen->format->Bmask, fbscreen->format->Amask);
  screen512 = SDL_CreateRGBSurface(0, 512, 512,
    16, fbscreen->format->Rmask, fbscreen->format->Gmask,
    fbscreen->format->Bmask, fbscreen->format->Amask);
  screen256 = SDL_CreateRGBSurface(0, 256, 256,
    16, fbscreen->format->Rmask, fbscreen->format->Gmask,
    fbscreen->format->Bmask, fbscreen->format->Amask);
#if !defined(BUILD_SNES)
  screen1024 = SDL_CreateRGBSurface(0, 1024, 1024,
    16, fbscreen->format->Rmask, fbscreen->format->Gmask,
    fbscreen->format->Bmask, fbscreen->format->Amask);
  screen = screen1024;
#else
  screen = screen512;
#endif /* BUILD_SNES */

  tex256buffer = calloc(1, 256*256*2);
  if (!tex256buffer) {
    fprintf(stderr, "\nERROR: Unable to allocate 256x256 texture buffer!\n");
    exit(1);
  }
  tex512buffer = calloc(1, 512*512*2);
  if (!tex512buffer) {
    fprintf(stderr, "\nERROR: Unable to allocate 512x512 texture buffer!\n");
    exit(1);
  }

  /* Shut off the mouse pointer */
  SDL_ShowCursor(SDL_DISABLE);

  /* Turn on EGL */
  EGLInitialize();
  EGLDestSize(fbscreen->w, fbscreen->h);
  /* Splash screen has to run in "normal" GUI size here */
#if !defined(BUILD_SNES)
  EGLSrcSize(720, 480);
#else
  EGLSrcSize(512, 512);
#endif /* BUILD_SNES */

  /* Init font engine */
  if(TTF_Init()==-1) {
    fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
    quit(2);
  }

#if defined(BUILD_SNES)
  /* Check for joysticks */
  BESResetJoysticks();

  /* Get SOMETHING on the screen ASAP */
  done = 0;
  SDL_FillRect(screen, NULL, 0x0);
  SDL_UpdateRect(screen, 0, 0, 0, 0);
  EGLBlitGL(screen->pixels);
  EGLFlip();

  /* Get everything loaded and setup */
  loadFonts();
  loadControlDatabase();
  BESProcessEvents();
#endif /* BUILD_SNES */

  return(0);
}
#if !defined(BUILD_SNES)
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
  audioAvailable = 1;
  initAudio();
}

void *loadingThreadFunc(void *)
{
  std::string ip;

  logo = IMG_Load("gfx/bes_logo_trans.png");
  gradient = IMG_Load("gfx/gradient.png");

  loadFonts();
  loadControlDatabase();
  loadGameDatabase();
  loadInstruct();
  loadGameLists();
  initGameInfo();
  loadAudio();
  loadPauseGui();
  loadGBAGui();
  if (vGameInfo.size() == 0)
    loadNoGamesGui();

  /* Render IP address */
  ip = "Current IP Address: ";
  ip += ipAddress;
  ipImage = TTF_RenderText_Blended(fontFSB16, ip.c_str(), textColor);
  printf("IP STRING: '%s'\n", ip.c_str());

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
  uint8_t frames;
  bool force = true;

  guiQuit = false;

  /* Get the screen set up again */
  EGLDestSize(fbscreen->w, fbscreen->h);
  EGLSrcSizeGui(720, 480, guiSize);
  done = 0;

  /* Clear out previous events */
  while ( SDL_PollEvent(&event) ) {}

  if (vGameInfo.size() == 0)
  {
    SDL_FillRect(screen, NULL, 0x0);
    SDL_UpdateRect(screen, 0, 0, 0, 0);
    doNoGamesGui();
    guiQuit = 1;
    done = 1;
    return 0;
  }

  /* Check for joysticks */
  BESResetJoysticks();

  /* Get SOMETHING on the screen ASAP */
  SDL_FillRect(screen, NULL, 0x0);
  SDL_BlitSurface(gradient, NULL, screen, &gradientRect);
  SDL_BlitSurface(logo, NULL, screen, &logoRect);
  SDL_BlitSurface(ipImage, NULL, screen, &ipRect);
  SDL_UpdateRect(screen, 0, 0, 0, 0);
  startAudio();

  /* In case the player saved a snapshot on the last game, redraw
    the cached game info panel. */
  force = true;

  while ( !done ) {
    gettimeofday(&startTime, NULL);

    renderGameList(screen);
    if (guiSize != GUI_SMALL)
    {
      renderGameInfo(screen, currentSelectedGameIndex(), force);
      renderInstruct(screen, BESJoystickPresent[0] | BESPRUGamepadPresent[0]);
      force = false;
    }

#if 0 /* CAPE_LCD3 */
    else
      renderVolume(screen);
#endif /* CAPE_LCD3 */

    EGLBlitGL(screen->pixels);
    EGLFlip();

    /* Consume GPIO/PRU input events and create key events */
    BESProcessEvents();

    /* Process regular input events */
    while ( SDL_PollEvent(&event) ) {
      switch (event.type) {
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
        case SDL_JOYAXISMOTION:
          /* Consume these events and create key events */
          BESProcessJoystickEvent(&event);
          break;

        case SDL_MOUSEBUTTONDOWN:
          break;

        case SDL_KEYUP:
          switch (event.key.keysym.sym) {
            case BES_P1_DU:
            case BES_P1_DD:
            case BES_P1_BL: /* Left trigger */
            case BES_P1_BR: /* Right trigger */
              menuPressDirection = 0;
              break;
            default:
              break;
          }
          break;

        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
#if 0 // AWH
            case BES_P1_DL:
              shiftSelectedVolumeUp();
              break;
            case BES_P1_DR:
              shiftSelectedVolumeDown();
              break;
#endif // AWH
            /* Gamepad left trigger */
            case BES_P1_BL:
              menuPressDirection = -1;
              menuPressDirectionStep = 6;
              break;
            /* Gamepad right trigger */
            case BES_P1_BR:
              menuPressDirection = 1;
              menuPressDirectionStep = 6;
              break;
            /* Gamepad up */
            case BES_P1_DU:
              menuPressDirection = -1;
              menuPressDirectionStep = 1;
              break;
            /* Gamepad down */
            case BES_P1_DD:
              menuPressDirection = 1;
              menuPressDirectionStep = 1;
              break;
            /* Gamepad select/start */
            case BES_P1_SE:
            case BES_P1_ST:
              if (acceptButton()) {
                playSelectSnd();
                done = 1;
              }
              break;
            case BES_QUIT:
              guiQuit = true;
              done = true;
              break;
            default:
              break;
          } // End keydown switch
          break;
        case SDL_QUIT:
fprintf(stderr, "SDL_QUIT\n");
          guiQuit = true;
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

    /* Get our end-of-frame time to see how long we spent doing everything */ 
    gettimeofday(&endTime, NULL);
    elapsedTime = ((endTime.tv_sec - startTime.tv_sec) * 1000000) + 
      (endTime.tv_usec - startTime.tv_usec);

    /* Skip the necessary number of logical frames to match wall clock time */
    frames = elapsedTime / TIME_PER_FRAME;
    incrementGameListFrame(frames + 1);
    usleep(elapsedTime % TIME_PER_FRAME);
  
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
    EGLBlitGL(screen->pixels);
    EGLFlip();
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

void renderVolume(SDL_Surface *surface) {
  SDL_Rect bar = {45, 170, 20, 30};
  if (!audioAvailable || volumeOverlayCount <= 0) return;

  for (int i = 0; i < (currentVolume >> 3); i++) {
    SDL_FillRect(surface, &bar, 0x07E0);
    bar.x += 30;
  }
  volumeOverlayCount--;
}
#endif /* BUILD_SNES */


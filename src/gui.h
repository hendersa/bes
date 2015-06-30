#ifndef __GUI_H__
#define __GUI_H__

#include <pthread.h>
#include <stdint.h>
#include "beagleboard.h"
// AWH #include "nes/driver.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Set the resolution at or under which you need to use the
  GUI for "small" screens */
#define GUI_SMALL_WIDTH 480
#define GUI_SMALL_HEIGHT 272

#define TARGET_FPS 24
#define TIME_PER_FRAME (1000000 / TARGET_FPS)

#if defined(BEAGLEBONE_BLACK)
#define BES_FILE_ROOT_DIR "/boot/uboot/bes"
#else
#define BES_FILE_ROOT_DIR "."
#endif 

#define BES_ROM_DIR "roms"
#define BES_SAVE_DIR "saves"
#define BES_SRAM_DIR "srams"

#define BES_SCREENSHOT_DIR "screenshot"
#define BES_GAMEBOY_SAVE_DIR "sgm"
#define BES_GAMEBOY_SRAM_DIR ""
#define BES_NES_SAVE_DIR "fcs"
#define BES_NES_SRAM_DIR "sav"

struct SDL_Surface;
struct _TTF_Font;
struct SDL_Color;
struct SDL_Rect;
union SDL_Event;
extern SDL_Surface *screen1024, *screen512, *screen256, *screenPause;
extern void *tex256buffer, *tex512buffer;
extern int guiQuit;
extern int texToUse;
extern int inPauseGui;
extern float screenshotWidth;
extern float screenshotHeight;
extern uint32_t BESPauseComboCurrent;

/* Various fonts for GUI/dialogs */
extern SDL_Surface *dlgMarker;
extern _TTF_Font *fontFSB12, *fontFSB16, *fontFSB25;
extern _TTF_Font *fontFS16, *fontFS20, *fontFS24, *fontFS25, *fontFS30;
extern SDL_Color textColor, inactiveTextColor;
enum {
        UL_CORNER = 0,
        UR_CORNER,
        LL_CORNER,
        LR_CORNER,
        TOP_BORDER,
        BOTTOM_BORDER,
        LEFT_BORDER,
        RIGHT_BORDER,
        TOTAL_PIECES
};
extern SDL_Rect guiPiece[TOTAL_PIECES];

/* GUI size: Small is for displays 480x272 or smaller, normal is for 
  all others */
typedef enum {
	GUI_SMALL = 0,
	GUI_NORMAL
} guiSize_t;
extern guiSize_t guiSize;


#define NUM_JOYSTICKS 2
#define PAUSE_GUI_WIDTH 300
#define PAUSE_GUI_HEIGHT 220

/* No audio dialog */
extern void doAudioDlg(void);

/* Map of js0, js1, etc. to the proper joystick (-1 if no joystick) */
extern int32_t BESDeviceMap[NUM_JOYSTICKS];
extern void BESResetJoysticks(void);
extern void BESCheckJoysticks(void);
extern void handleJoystickEvent(SDL_Event *event);
extern uint32_t BESControllerPresent[NUM_JOYSTICKS];
extern uint32_t BESPauseCombo;

extern int doGuiSetup(void);
extern int doGui(void);
extern void loadGameInfo(void);
extern void loadGameLists(void);
extern void loadInstruct(void);
extern void loadAudio(void);

/* GPIO */
/* Four axis buttons, eight controller buttons, one pause */
#define GPIO_MAP_SIZE 13
extern uint32_t gpioPinSetup(void);
extern void gpioEvents(void);

/* guiParser.c */
extern int loadGameConfig(void);

extern void renderInstruct(SDL_Surface *screen, 
  const uint32_t gamepadPresent);
extern void renderGameList(SDL_Surface *screen);
extern void renderGameInfo(SDL_Surface *screen, const uint32_t i);
#if defined (CAPE_LCD3)
extern void renderVolume(const SDL_Surface *surface);
#endif /* CAPE_LCD3 */
extern uint32_t currentSelectedGameIndex(void);
extern void incrementGameListFrame(void);
extern void shiftSelectedGameUp(const int step);
extern void shiftSelectedGameDown(const int step);
extern void enableGuiAudio(void);
extern void disableGuiAudio(void);
extern void initAudio(void);
extern void startAudio(void);
extern void fadeAudio(void);
extern void playSelectSnd(void);
extern void playOverlaySnd(void);
extern void playTeleSnd(void);
extern void playCoinSnd(void);
extern void changeVolume(void);
extern uint32_t acceptButton(void);
 
#define DEFAULT_BOX_IMAGE "box_image.png"
#define DEFAULT_DATE_TEXT "19XX"
#define MAX_GENRE_TYPES 2
#define MAX_TEXT_LINES 5

#define GAME_TITLE_SIZE 64
#define ROM_FILE_SIZE 128
#define IMAGE_FILE_SIZE 128
#define INFO_TEXT_SIZE 64
#define DATE_TEXT_SIZE 5
#define GENRE_TEXT_SIZE 32

/* Snapshot load screen */
enum {
  /* No snapshot for this game */
  SNAPSHOT_LOAD_NONE = 0,
  /* Snapshot available */
  SNAPSHOT_LOAD_SKIP,
  SNAPSHOT_LOAD_RESTORE,
  SNAPSHOT_LOAD_DELETE
};

enum {
  PLAYER_INVALID = -1,
  PLAYER_ONE = 0,
  PLAYER_TWO,
  NUM_PLAYERS
};

/* Define the XML tags used in the games.xml */
typedef enum {
  /* Start! */
  TAG_FIRST = 0,
  /* Root config tag */
  TAG_CONFIG = TAG_FIRST,
  /* Platform tags */
  TAG_FIRST_PLATFORM,
  TAG_SNES = TAG_FIRST_PLATFORM,
  TAG_GBA,
  TAG_NES,
  TAG_GBC,
  TAG_LAST_PLATFORM = TAG_GBC,
  /* Game menu tags */
  TAG_GAME_FIRST,
  TAG_GAME = TAG_GAME_FIRST,
  TAG_TITLE,
  TAG_ROM,
  TAG_IMAGE,
  TAG_YEAR,
  /* There can be multiple of these per "game" tag */
  TAG_GENRE,
  TAG_TEXT,
  TAG_GAME_LAST = TAG_TEXT,
  /* Controller config tags */
  TAG_PLAYER1,
  TAG_PLAYER2,
  TAG_FIRST_CONTROL,
  TAG_VAXIS = TAG_FIRST_CONTROL,
  TAG_HAXIS,
  TAG_FIRST_BUTTON,
  TAG_LEFT = TAG_FIRST_BUTTON,
  TAG_RIGHT,
  TAG_A,
  TAG_B,
  TAG_X,
  TAG_Y,
  TAG_SELECT,
  TAG_START,
  TAG_PAUSE,
  TAG_LAST_CONTROL = TAG_PAUSE,
  /* GPIO control mapping tags */
  TAG_GPIO,
  TAG_FIRST_GPIO_CONTROL,
  TAG_GPIO_GPLEFT = TAG_FIRST_GPIO_CONTROL,
  TAG_GPIO_GPRIGHT,
  TAG_GPIO_GPUP,
  TAG_GPIO_GPDOWN,
  TAG_GPIO_LEFT,
  TAG_GPIO_RIGHT,
  TAG_GPIO_A,
  TAG_GPIO_B,
  TAG_GPIO_X,
  TAG_GPIO_Y,
  TAG_GPIO_SELECT,
  TAG_GPIO_START,
  TAG_GPIO_PAUSE,
  /* Pause key and key combo */
  TAG_PAUSE_COMBO,
  TAG_PAUSE_KEY,
  /* Done! */
  TAG_LAST
} TagUsed_t;

typedef enum {
  PLATFORM_INVALID = -1,
  PLATFORM_FIRST = TAG_FIRST_PLATFORM,
  PLATFORM_SNES = PLATFORM_FIRST,
  PLATFORM_GBA,
  PLATFORM_NES,
  PLATFORM_GBC,
  NUM_PLATFORMS
} platformType_t;

/* Eight buttons, one pause, and two axis */
#define BUTTON_MAP_SIZE 11

extern unsigned char BESButtonMap[NUM_PLAYERS][BUTTON_MAP_SIZE];
extern unsigned char BESAxisMap[NUM_PLAYERS][2][2];
extern unsigned char GPIOButtonMap[GPIO_MAP_SIZE];

/* Linked list node for game information */
typedef struct _gameInfo {
  /* The name of the game */
  char gameTitle[GAME_TITLE_SIZE];
  /* Filename of the ROM image */
  char romFile[ROM_FILE_SIZE];
  /* Filename of the image of the game's box */
  char imageFile[IMAGE_FILE_SIZE];
  /* Lines of text that describe the game */
  char infoText[MAX_TEXT_LINES][INFO_TEXT_SIZE];
  /* Four digit year the game was released */
  char dateText[DATE_TEXT_SIZE];
  /* Short descriptive genre text */
  char genreText[MAX_GENRE_TYPES][GENRE_TEXT_SIZE];
  /* Platform this game runs on */
  int platform;
  /* Link to next game in list */
  struct _gameInfo *next;
  /* Link to prev game in list */
  struct _gameInfo *prev;
} gameInfo_t;

extern gameInfo_t *gameInfo;
extern int totalGames;
extern int currentPlatform;

extern int audioAvailable;

extern int currentVolume;
extern int volumePressDirection;
extern int volumeOverlayCount;

extern void shiftSelectedVolumeUp(void);
extern void shiftSelectedVolumeDown(void);

extern int selectButtonNum;
extern int startButtonNum;

extern int emuDone;

/* eglSetup.c */
extern void EGLShutdown(void);
extern void EGLFlip(void);
extern int EGLInitialize(void);

/* eglTexture.c */
enum {
  TEXTURE_256 = 0,
  TEXTURE_512 = 1,
  TEXTURE_1024 = 2,
  TEXTURE_PAUSE = 3,
  TEXTURE_SCREENSHOT = 4,
  TEXTURE_LAST = 5
};

extern void EGLSrcSize(const uint32_t width, const uint32_t height);
extern void EGLSrcSizeGui(const uint32_t width, const uint32_t height, 
  const guiSize_t smallGui);
extern void EGLDestSize(const uint32_t width, const uint32_t height);
extern void EGLSetupGL(void);
extern void EGLBlitGL(const void *buf);
extern void EGLBlitGLCache(const void *buf, const int cache);
extern void EGLBlitPauseGL(const void *buf, const float pauseX, 
  const float pauseY, const float scaleX, const float scaleY);
extern void EGLBlitGBAGL(const void *buf, const float pauseX, 
  const float pauseY, const float scaleX, const float scaleY);
#ifdef __cplusplus
}
#endif /* __cplusplus */

/* Emulator launch functions */
extern int gba_main(const char *filename);
extern int nes_main(const char *filename);
extern int snes_main(const char *filename);

/* Loading background thread */
extern pthread_t loadingThread;
extern void doGuiLoadingThread(void);

/* Splash screen */
extern void loadSplashAudio(void);
extern void doSplashScreen(void);

/* GBA setting GUI */
extern void loadGBAGui(void);
extern void doGBAGui(void);
extern int gbaForceSettings;
enum {
  GBA_FORCE_FS_SYNC_AUDIO = 0,
  GBA_FORCE_NO_FS_AUDIO,
  GBA_FORCE_NO_FS_NO_AUDIO,
  GBC_FORCE
};

/* Pause GUI */
extern void loadPauseGui(void);
extern uint32_t doPauseGui(const char *romname, 
  const platformType_t platform);
enum {
  PAUSE_NONE = 0, /* No special pause activity */
  PAUSE_NEXT,  /* On the next pass through, pause */
  PAUSE_CACHE, /* Cache the texture, then switch to pause next */
  PAUSE_CACHE_NO_DRAW, /* Cache the texture, but don't flip */
  PAUSE_IN_DIALOG /* In the pause dialog already when caching */ 
};
extern void saveScreenshot(const char *romname);
extern void loadScreenshot(const char *romname);
extern int BESPauseState;

/* VBAM emulator functions */
extern void sdlWriteState(int num);
extern void sdlReadState(int num);
extern bool systemPauseOnFrame(void);
#endif /* __GUI_H__ */


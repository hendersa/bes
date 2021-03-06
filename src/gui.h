#ifndef __GUI_H__
#define __GUI_H__

#include <pthread.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "beagleboard.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Set the resolution at or under which you need to use the
  GUI for "small" screens */
#define GUI_SMALL_WIDTH 480
#define GUI_SMALL_HEIGHT 272

#define TARGET_FPS 60
#define TIME_PER_FRAME (1000000 / TARGET_FPS)

#if defined(BEAGLEBONE_BLACK)
#define BES_FILE_ROOT_DIR "/home/ubuntu/bes"
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
extern bool guiQuit;
extern int texToUse;
extern bool inPauseGui;
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


#define PAUSE_GUI_WIDTH 300
#define PAUSE_GUI_HEIGHT 220

/* guiAudioDialog.cpp */
extern void doAudioDlg(void);

/* Map of js0, js1, etc. to the proper joystick (-1 if no joystick) */
//extern void BESResetJoysticks(void);
//extern void BESCheckJoysticks(void);
//extern void handleJoystickEvent(const SDL_Event *event);
//extern uint32_t BESControllerPresent[NUM_JOYSTICKS];
//extern uint32_t BESPauseCombo;

extern int doGuiSetup(void);
extern int doGui(void);

/* guiGameList.cpp */
extern void loadGameLists(void);
extern void renderGameList(SDL_Surface *screen);
extern uint32_t currentSelectedGameIndex(void);
extern void incrementGameListFrame(const uint8_t frames);
extern void shiftSelectedGameUp(const int step);
extern void shiftSelectedGameDown(const int step);
extern uint32_t acceptButton(void);

/* guiGamepad.cpp */
extern void loadInstruct(void);
extern void renderInstruct(SDL_Surface *screen, const uint32_t gamepadPresent);

/* guiGameInfo.cpp */
extern void initGameInfo(void);
extern void renderGameInfo(SDL_Surface *screen, const uint32_t index, const bool force);

/* gui.cpp */
extern void renderVolume(const SDL_Surface *surface);
extern void enableGuiAudio(void);
extern void disableGuiAudio(void);
extern bool audioAvailable;

/* guiAudio.cpp */
extern void loadAudio(void);
extern void loadWelcomeAudio(void);
extern void initAudio(void);
extern void startAudio(void);
extern void startWelcomeAudio(void);
extern void fadeAudio(void);
extern void playSelectSnd(void);
extern void playOverlaySnd(void);
extern void playTeleSnd(void);
extern void playCoinSnd(void);
extern void changeVolume(void);
 
#define DEFAULT_BOX_IMAGE "box_image.png"
#define DEFAULT_DATE_TEXT "19XX"
#define MAX_GENRE_TYPES 2
#define MAX_TEXT_LINES 5

typedef enum {
  PLATFORM_INVALID = -1,
  PLATFORM_FIRST = 0,
  PLATFORM_SNES = PLATFORM_FIRST,
  PLATFORM_GBA,
  PLATFORM_NES,
  PLATFORM_GBC,
  PLATFORM_LAST = PLATFORM_GBC,
  PLATFORM_TOTAL
} enumPlatform_t;

/* Linked list node for game information */
typedef struct _gameInfo {
  /* The name of the game */
  std::string gameTitle;
  /* Filename of the ROM image */
  std::string romFile;
  /* Lines of text that describe the game */
  std::string infoText[MAX_TEXT_LINES];
  /* Four digit year the game was released */
  std::string dateText;
  /* Short descriptive genre text */
  std::string genreText[MAX_GENRE_TYPES];
  /* Platform this game runs on */
  enumPlatform_t platform;
} gameInfo_t;

extern std::vector<gameInfo_t> vGameInfo;

extern uint8_t currentVolume;
extern int volumePressDirection;
extern int volumeOverlayCount;

extern void shiftSelectedVolumeUp(void);
extern void shiftSelectedVolumeDown(void);

extern int8_t selectButtonNum;
extern int8_t startButtonNum;

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

/* guiPauseDlg.cpp */
extern void loadPauseGui(void);
extern uint32_t doPauseGui(const char *romname, const enumPlatform_t platform);
extern int checkForSnapshot(const char *romname, const int platform,
	SDL_Surface *snapshot, const uint16_t width, const uint16_t height);
typedef enum {
  PAUSE_NONE = 0, /* No special pause activity */
  PAUSE_NEXT,  /* On the next pass through, pause */
  PAUSE_CACHE, /* Cache the texture, then switch to pause next */
  PAUSE_CACHE_NO_DRAW, /* Cache the texture, but don't flip */
  PAUSE_IN_DIALOG /* In the pause dialog already when caching */ 
} pauseState_t;
extern pauseState_t BESPauseState;
extern void *tex256buffer, *tex512buffer;

/* besDatabase.cpp */
extern bool loadGameDatabase(void);
extern bool loadControlDatabase(void);
extern bool loadGPIODatabase(void);

/* guiNoGamesDlg.cpp */
extern void loadNoGamesGui(void);
extern uint32_t doNoGamesGui(void);

#endif /* __GUI_H__ */


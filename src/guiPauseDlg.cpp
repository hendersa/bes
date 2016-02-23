/*****************************************************************************
  BeagleSNES - Super Nintendo Entertainment System (TM) emulator for the
  BeagleBoard-xM platform.

  See CREDITS file to find the copyright owners of this file.

  GUI front-end code (c) Copyright 2013 Andrew Henderson (hendersa@icculus.org)

  BeagleSNES homepage: http://www.beaglesnes.org/
  
  BeagleSNES is derived from the Snes9x open source emulator:  
  Snes9x homepage: http://www.snes9x.com/

  Permission to use, copy, modify and/or distribute BeagleSNES in both binary
  and source form, for non-commercial purposes, is hereby granted without
  fee, providing that this license information and copyright notice appear
  with all copies and any derived work.

  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software or it's derivatives.

  BeagleSNES is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes,
  but is not limited to, charging money for BeagleSNES or software derived 
  from BeagleSNES, including BeagleSNES or derivatives in commercial game 
  bundles, and/or using BeagleSNES as a promotion for your commercial 
  product.

  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.

  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
 *****************************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <fcntl.h>
#include "gui.h"
#include "beagleboard.h"
#include "savepng.h"

#if !defined (BUILD_SNES)
/* NES: Nestopia */
#include "nes/unix/auxio.h"
/* GBA: VBAM */
extern void sdlWriteState(int num);
extern void sdlReadState(int num);
extern bool systemPauseOnFrame(void);
#else
/* SNES: SNES9X */
#include "snes9x.h"
#include "display.h"
#include "snapshot.h"
#include "gfx.h"
#endif /* BUILD_SNES */

pauseState_t BESPauseState = PAUSE_NONE;

void *tex256buffer = NULL;
void *tex512buffer = NULL;

static SDL_Surface *pauseGui = NULL;

static uint32_t bgColor;
static const char *headerText = "BeagleSNES PAUSED";
static const char *menuText[5] = { "Resume Game", "Load Snapshot", 
	"Save Snapshot", "Return To Game Menu", "LAST SNAPSHOT" }; 
static SDL_Surface *menuImage[7] = { 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL }; 
static SDL_Rect menuTextPos[7] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0},
	{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };

static uint32_t snapshotAvailable = 0;
/* These two are loaded/saved from/to disk */
static SDL_Surface *noSnapshotImage = NULL;
static SDL_Surface *snapshotImage = NULL;
/* This is the tiny 128x96 one */
static SDL_Surface *tinySnapshotImage = NULL;

static SDL_Surface *pauseMenuSurface = NULL;
//static uint32_t menuPressDirection = 0;
static uint32_t forceUnpause = 0, done = 0;
static uint32_t frameCounter = 0, currentIndex = 0, nextIndex = 0;
static SDL_Rect markerPos[4] = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
//static SDL_Surface *currentScreen = NULL;

#if 0
#if defined(CAPE_LCD3)
static const uint32_t xOffset = 10; // (320-300)/2;
static const uint32_t yOffset = 10; // (240-220)/2;
#else
static const uint32_t xOffset = 210; // (720-300)/2;
static const uint32_t yOffset = 130; // (480-220)/2;
#endif /* CAPE_LCD3 */
#endif
static void renderPauseGui(const char *romname, int platform);
static void shiftSelectedItemUp(void);
static void shiftSelectedItemDown(void);
static void incrementPauseItemFrame(void);
static void loadScreenshot(const char *romname, int platform);
static void saveScreenshot(const char *romname, int platform);
static void checkForSnapshot(const char *romname, int platform);

void loadPauseGui(void) {
	SDL_Surface *tempSurface;
	SDL_PixelFormat *format;

	format = screenPause->format;

	/* Load the graphics for the pause menu */
	tempSurface = IMG_Load("gfx/pausemenu_320x240.png");
	if (!tempSurface) {
		fprintf(stderr, "\nERROR: Unable to load Pause GUI images\n");
		exit(1);
	}
	pauseGui = SDL_ConvertSurface(tempSurface, format, 0);
	SDL_FreeSurface(tempSurface);

	bgColor = SDL_MapRGB(screenPause->format, 0x28, 0x28, 0x80);
	noSnapshotImage = IMG_Load("gfx/blank_snapshot.png");
	if (!noSnapshotImage)
		fprintf(stderr, "Unable to load Pause GUI blank snapshot image\n");

	/* Render menu text */
	menuImage[0] = TTF_RenderText_Blended(fontFSB25, headerText,
		textColor);
	menuImage[1] = TTF_RenderText_Blended(fontFSB16, menuText[0],
		textColor);
	menuImage[2] = TTF_RenderText_Blended(fontFSB16, menuText[1],
		textColor);
	menuImage[3] = TTF_RenderText_Blended(fontFSB16, menuText[2],
		textColor);
	menuImage[4] = TTF_RenderText_Blended(fontFSB16, menuText[3],
		textColor); 
	menuImage[5] = TTF_RenderText_Blended(fontFSB16, menuText[1],
		inactiveTextColor);
	menuImage[6] = TTF_RenderText_Blended(fontFSB12, menuText[4],
		textColor);
}

void renderPauseGui(const char *romname, int platform) {
	SDL_Rect bgRect = {0,0,(PAUSE_GUI_WIDTH - 8), (PAUSE_GUI_HEIGHT-8)};
	SDL_Rect pos = {0,0,0,0};
	uint32_t i;

	/* Check if there is a snapshot available */
	checkForSnapshot(romname, platform);

	menuTextPos[0].x = (PAUSE_GUI_WIDTH / 2) - (menuImage[0]->w / 2); 
	menuTextPos[0].y = 8;
	
	for (i=1; i < 5; i++)
	{
		menuTextPos[i].x = 19;
		menuTextPos[i].y = 32 + (i * 36);
		markerPos[i-1].x = 5;
		markerPos[i-1].y = 32 + (i * 36);
	}
	menuTextPos[5].x = menuTextPos[2].x;
	menuTextPos[5].y = menuTextPos[2].y;
	menuTextPos[6].x = 176;
	menuTextPos[6].y = 52; 

	bgRect.x = bgRect.y = 4;

	/* Corners */
	SDL_SetColorKey(pauseGui, SDL_SRCCOLORKEY, SDL_MapRGB(screenPause->format, 0xFF, 0x00, 0xFF));
	pos.x = pos.y = 0;
	SDL_BlitSurface(pauseGui, &(guiPiece[UL_CORNER]), screenPause, &pos);	
	pos.y = PAUSE_GUI_HEIGHT - 8;
	SDL_BlitSurface(pauseGui, &(guiPiece[LL_CORNER]), screenPause, &pos);
	pos.x = PAUSE_GUI_WIDTH - 8; pos.y = 0;
	SDL_BlitSurface(pauseGui, &(guiPiece[UR_CORNER]), screenPause, &pos);
	pos.y = PAUSE_GUI_HEIGHT - 8;
	SDL_BlitSurface(pauseGui, &(guiPiece[LR_CORNER]), screenPause, &pos);
	SDL_SetColorKey(pauseGui, SDL_FALSE, 0);

	/* Top */
	pos.y = 0;
	for (i=0; i < 36; i++)
	{
		pos.x = 8 + (i*8);
		SDL_BlitSurface(pauseGui, &guiPiece[TOP_BORDER], screenPause, &pos);
	}

	/* Bottom */
	pos.y = PAUSE_GUI_HEIGHT - 4;
	for (i=0; i < 36; i++)
	{     
		pos.x = 8 + (i*8);
		SDL_BlitSurface(pauseGui, &guiPiece[BOTTOM_BORDER], screenPause, &pos);
	}

	/* Left */
	pos.x = 0;
	for (i=0; i < 26; i++)
	{     
		pos.y = 8 + (i*8);
		SDL_BlitSurface(pauseGui, &guiPiece[LEFT_BORDER], screenPause, &pos);
	}

	/* Right */
	pos.x = PAUSE_GUI_WIDTH - 4;
	for (i=0; i < 26; i++)
	{     
		pos.y = 8 + (i*8);
		SDL_BlitSurface(pauseGui, &guiPiece[RIGHT_BORDER], screenPause, &pos);
	}
	
	/* Background */
	SDL_FillRect(screenPause, &bgRect, bgColor); 

	/* Menu text */
	SDL_BlitSurface(menuImage[0], NULL, screenPause, &(menuTextPos[0]));
	SDL_BlitSurface(menuImage[1], NULL, screenPause, &(menuTextPos[1]));
	for (i=3; i < 5; i++)
		SDL_BlitSurface(menuImage[i], NULL, screenPause, &(menuTextPos[i]));
	SDL_BlitSurface(menuImage[6], NULL, screenPause, &(menuTextPos[6]));

	/* Determine if the snapshot loading is enabled */
	pos.x = 166;
	pos.y = 70;

	if (snapshotAvailable)
	{
		SDL_BlitSurface(menuImage[2], NULL, screenPause, &(menuTextPos[2]));
		SDL_BlitSurface(tinySnapshotImage, NULL, screenPause, &pos);
	}
	else
	{
		SDL_BlitSurface(menuImage[5], NULL, screenPause, &(menuTextPos[2]));
		SDL_BlitSurface(noSnapshotImage, NULL, screenPause, &pos);
	}

  /* Finally, now that we have rendered everything, cache a copy of it */
	pos.x = pos.y = 0; 
	pos.w = PAUSE_GUI_WIDTH; pos.h = PAUSE_GUI_HEIGHT;
	pauseMenuSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, PAUSE_GUI_WIDTH,
		PAUSE_GUI_HEIGHT, 16, 0x7C00, 0x03E0, 0x001F, 0);
	SDL_BlitSurface(screenPause, &pos, pauseMenuSurface, NULL); 
}

uint32_t doPauseGui(const char *romname, const platformType_t platform)
{
	SDL_Rect menuPos = {0,0,0,0};
	struct timeval startTime, endTime;
	uint64_t elapsedTime;
	SDL_Event event;
	uint32_t returnVal = 0;
	uint8_t i;
	float dialogXPos, dialogYPos, scaleX, scaleY;
	void *buffer;
	BESPauseComboCurrent = 0;

	switch(texToUse)
	{
		case TEXTURE_256:
			buffer = tex256buffer; break;
		case TEXTURE_512:
			buffer = tex512buffer; break;
		default:
			buffer = NULL; break;
	}
 
	snapshotAvailable = 0;
	// SNES9X Settings.Paused = 1;
	
	tinySnapshotImage = SDL_CreateRGBSurface(SDL_SWSURFACE, 128,
		96, 16, 0xFC00, 0x07E0, 0x001F, 0);	
	/* Render the pause menu on the screen and make a copy of it */
	renderPauseGui(romname, platform);

	done = 0; forceUnpause = 0;
	nextIndex = currentIndex = frameCounter = 0;

	/* Do slide-in animation */
	dialogXPos = 1.75f;
	dialogYPos = 0.0f;
	if (guiSize == GUI_NORMAL)
		scaleX = scaleY = 0.5f;
	else 
		scaleX = scaleY = 1.0f;

	SDL_BlitSurface(pauseMenuSurface, NULL, screenPause, &menuPos);
	for(i = 0; i < 35; i++)
	{
		gettimeofday(&startTime, NULL);
		if (buffer)
			EGLBlitGL(buffer);
		EGLBlitPauseGL(screenPause->pixels, dialogXPos, dialogYPos, scaleX, scaleY);
		dialogXPos -= 0.05f;
		EGLFlip();
		gettimeofday(&endTime, NULL);
		elapsedTime = ((endTime.tv_sec - startTime.tv_sec) * 1000000) +
			(endTime.tv_usec - startTime.tv_usec);
		if (elapsedTime < (TIME_PER_FRAME / 8))
			usleep((TIME_PER_FRAME / 8) - elapsedTime);
	}
 
	/* Do drawing and animation loop */
	while (!done) {
		gettimeofday(&startTime, NULL);

		/* Render the pause menu */
		SDL_BlitSurface(pauseMenuSurface, NULL, screenPause, &menuPos);
		/* Render the marker */
		incrementPauseItemFrame();
		SDL_BlitSurface(dlgMarker, NULL, screenPause, &(markerPos[nextIndex]));
		if (buffer)
			EGLBlitGL(buffer);
		EGLBlitPauseGL(screenPause->pixels, dialogXPos, dialogYPos, scaleX, scaleY);
		EGLFlip();

		gpioEvents();

		/* Check for events */
		while ( SDL_PollEvent(&event) ) {
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN:
					break;

				case SDL_JOYBUTTONDOWN:
				case SDL_JOYBUTTONUP:
				case SDL_JOYAXISMOTION:
					handleJoystickEvent(&event);
					break;

				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_UP:
							shiftSelectedItemUp();
							break;
						case SDLK_DOWN:
							shiftSelectedItemDown();
							break;
						case SDLK_r:
							forceUnpause = 1;
						case SDLK_RETURN:
							done = 1;
							break;
						default:
							break;
					} /* Exit keydown switch */
					break;
			
				case SDL_QUIT:
					fprintf(stderr, "SDL_QUIT\n");
					done = 1;
					break;

				default:
					break;
			} /* End eventtype switch */
		} /* End PollEvent loop */

		BESCheckJoysticks();
		gettimeofday(&endTime, NULL);
		elapsedTime = ((endTime.tv_sec - startTime.tv_sec) * 1000000) +
			(endTime.tv_usec - startTime.tv_usec);
		if (elapsedTime < TIME_PER_FRAME)
			usleep(TIME_PER_FRAME - elapsedTime);

		/* Are we really done? */
		if (done)
		{
			if (forceUnpause) /* Pause menu button pressed */
				nextIndex = 0;

			if (nextIndex == 1) { /* Load snapshot */
				switch(platform) {
#if !defined(BUILD_SNES)
					case PLATFORM_GBA:
					case PLATFORM_GBC:
						BESPauseState = PAUSE_CACHE_NO_DRAW;
						sdlReadState(0);
						systemPauseOnFrame();
						break;
					case PLATFORM_NES:
						BESPauseState = PAUSE_IN_DIALOG;
						auxio_do_state_load();
						break;
#else
					case PLATFORM_SNES:
					{
						std::string temp;
						temp = BES_FILE_ROOT_DIR "/" BES_SAVE_DIR "/snes/"; 
						temp += romname;
						temp += ".000";
						S9xUnfreezeGame(temp.c_str());
						//Settings.Paused = 1;
					}
					break;
#endif /* BUILD_SNES */
					default:
						break;
				}
				loadScreenshot(romname, platform);
				/* Resolution may have changed */				  
				switch(texToUse)
				{
					case TEXTURE_256:
						buffer = tex256buffer; break;
					case TEXTURE_512:
						buffer = tex512buffer; break;
					default:
						buffer = NULL; break;
				}

				done = 0;
			} else if (nextIndex == 2) { /* Save snapshot */
				switch(platform) {
#if !defined(BUILD_SNES)
					case PLATFORM_GBA:
					case PLATFORM_GBC:
						sdlWriteState(0);
						break;
					case PLATFORM_NES:
						auxio_do_state_save();
						break;
#else
					case PLATFORM_SNES:
					{
						int fdfile;
						std::string temp;
						/* SNES9X quicksave logic */
						temp = BES_FILE_ROOT_DIR "/" BES_SAVE_DIR "/snes/";
						temp += romname;
						temp += ".000";
						fprintf(stderr, "AWH: Save '%s'\n", temp.c_str());
						S9xFreezeGame(temp.c_str());
						fprintf(stderr, "After S9xFreezeGame()\n");
						fdfile = open(temp.c_str(), O_RDWR);
						if (fdfile > 0)
						{
							fsync(fdfile);
							close(fdfile);
						}
					}
					break;
#endif /* BUILD_SNES */
					default:
						break;
				}
				saveScreenshot(romname, platform);
				renderPauseGui(romname, platform);
				done = 0;
			} else if (nextIndex == 3) { /* Quit to menu */
				returnVal = 1;
			} else { /* Exit from pause screen */
			}	
		}
	} /* End while loop */

	/* Slide back out */
  SDL_BlitSurface(pauseMenuSurface, NULL, screenPause, &menuPos);
  for(i = 0; i < 45; i++)
  {
    gettimeofday(&startTime, NULL);
    if (buffer)
      EGLBlitGL(buffer);
    EGLBlitPauseGL(screenPause->pixels, dialogXPos, dialogYPos, scaleX, scaleY);
    dialogXPos += 0.05f;
    EGLFlip();
    gettimeofday(&endTime, NULL);
    elapsedTime = ((endTime.tv_sec - startTime.tv_sec) * 1000000) +
      (endTime.tv_usec - startTime.tv_usec);
    if (elapsedTime < (TIME_PER_FRAME / 8))
      usleep((TIME_PER_FRAME / 8) - elapsedTime);
  }

  /* Flush out any remaining events */
  while ( SDL_PollEvent(&event) );
	fprintf(stderr, "Pause GUI return val: %d\n", returnVal);
  BESPauseComboCurrent = 0;
	return returnVal;
}

static void shiftSelectedItemDown(void)
{
	/* Already shifting to a new index? */
	if (currentIndex != nextIndex) return;

	/* Are we at the end of the list? */
	if (currentIndex == 3) return;

	/* Start the shifting process */
	nextIndex = currentIndex + 1;

	/* Are we skipping the "load snapshot"? */
	if ((nextIndex == 1) && !snapshotAvailable)
		nextIndex++;
}

static void shiftSelectedItemUp(void)
{
	/* Already shifting to a new index? */
	if (currentIndex != nextIndex) return;

	/* Are we at the start of the list? */
	if (currentIndex == 0) return;

	/* Start the shifting process */
	nextIndex = currentIndex - 1;
	if ((nextIndex == 1) && !snapshotAvailable)
		nextIndex--;
}

void incrementPauseItemFrame(void)
{
	if (currentIndex != nextIndex)
		frameCounter++;

	/* After 6 frames (0.15 seconds), you can jump to the
	  next pause menu item */
	if (frameCounter == 6) {
		currentIndex = nextIndex;
		frameCounter = 0;
	}
}

/* controls.cpp */
void loadScreenshot(const char *romname, int platform) {
	std::string temp;
	SDL_Surface *tempSurface;
	SDL_PixelFormat *format = screenPause->format;

	/* Load the screenshot associated with this snapshot */
        temp = BES_FILE_ROOT_DIR "/" BES_SAVE_DIR "/";
	switch (platform)
	{
		case PLATFORM_GBA:
		case PLATFORM_GBC:
			temp += "gb/";
			break;
		case PLATFORM_NES:
			temp += "nes/";
			break;
		case PLATFORM_SNES:
			temp += "snes/";
			break;	
	}
	temp += romname;
	temp += ".png";

	if (snapshotImage) SDL_FreeSurface(snapshotImage);
	fprintf(stderr, "loadSnapshot(%s), '%s'\n", romname, temp.c_str());
	tempSurface = IMG_Load(temp.c_str());
	if (tempSurface) {
		snapshotImage = SDL_ConvertSurface(tempSurface, format, 0);
		SDL_FreeSurface(tempSurface);
		/* The screen resolution may have changed */
		EGLSrcSize(snapshotImage->w, snapshotImage->h);
		/* Render the background of the loaded snapshot */
		switch (texToUse) {
			case TEXTURE_256:
				SDL_BlitSurface(snapshotImage, NULL, screen256, NULL);
				EGLBlitGLCache(screen256->pixels, 1);
				break;
			case TEXTURE_512:
				SDL_BlitSurface(snapshotImage, NULL, screen512, NULL);
				EGLBlitGLCache(screen512->pixels, 1);
				break;
#if !defined(BUILD_SNES)
			default:
				SDL_BlitSurface(snapshotImage, NULL, screen1024, NULL);
				EGLBlitGLCache(screen1024->pixels, 1);
				break;
#endif /* BUILD_SNES */
		} 
	}
	 	
	/* Re-render the pause menu */
	renderPauseGui(romname, platform);
}

void saveScreenshot(const char *romname, int platform) {
	std::string imagePath, dirPath;
	SDL_PixelFormat *format;
	int fdfile;
	SDL_Surface *tempSurface;

	dirPath = BES_FILE_ROOT_DIR "/" BES_SAVE_DIR "/";
	switch (platform)
	{
		case PLATFORM_GBA:
		case PLATFORM_GBC:
			dirPath += "gb";
			break;
		case PLATFORM_NES:
			dirPath += "nes";
			break;
		case PLATFORM_SNES:
			dirPath += "snes";
			break;
	}
	imagePath = dirPath + "/" + romname + ".png";

	fprintf(stderr, "saveScreenshot(%s), '%s'\n", romname, imagePath.c_str());
	if (snapshotImage) SDL_FreeSurface(snapshotImage);
	format = screen512->format;
	switch(texToUse) {
		case TEXTURE_256:
			tempSurface = SDL_CreateRGBSurfaceFrom(tex256buffer,
				(int)screenshotWidth, (int)screenshotHeight, 16, 512,
				format->Rmask, format->Gmask, format->Bmask, format->Amask);
			snapshotImage = SDL_CreateRGBSurface(0, (int)screenshotWidth,
				(int)screenshotHeight, 32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
			SDL_BlitSurface(tempSurface, NULL, snapshotImage, NULL);
			break;
		case TEXTURE_512:
			tempSurface = SDL_CreateRGBSurfaceFrom(tex512buffer,
				(int)screenshotWidth, (int)screenshotHeight, 16, 1024,
				format->Rmask, format->Gmask, format->Bmask, format->Amask);
			snapshotImage = SDL_CreateRGBSurface(0, (int)screenshotWidth,
				(int)screenshotHeight, 32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000);
			SDL_BlitSurface(tempSurface, NULL, snapshotImage, NULL);
			break;
		default:
			snapshotImage = NULL;
			break;
	}

	if (snapshotImage)
		SDL_SavePNG(snapshotImage, imagePath.c_str());
	fdfile = open(imagePath.c_str(), 0, O_RDWR);
	if (fdfile > 0)
	{
		fsync(fdfile);
		close(fdfile);
		fdfile = open(dirPath.c_str(), O_RDWR);
		fsync(fdfile);
		close(fdfile);
	}
}

void checkForSnapshot(const char *romname, int platform)
{
	struct stat fileinfo;
	std::string temp;
	uint32_t dstRow, dstCol;
	uint16_t *srcPixel, *dstPixel;
	float xStep, yStep;
	size_t stringSize;
	SDL_Surface *tempSurface;

	snapshotAvailable = 1;
	/* Check for a ROM snapshot */
	temp = BES_FILE_ROOT_DIR "/" BES_SAVE_DIR "/";
	switch (platform)
	{
		case PLATFORM_SNES:
			temp += "snes/";
			temp += romname; 
			temp += ".000";
			break;
		case PLATFORM_GBA:
		case PLATFORM_GBC:
			temp += "gb/";
			temp += romname;
			temp += "1.sgm";
			break;
		case PLATFORM_NES:
			temp += "nes/";
			temp += romname;
			stringSize = temp.length();
			temp.resize(stringSize - 4);
			temp += ".nst";
			///sprintf(temp, "%s/%s/nes/%s", BES_FILE_ROOT_DIR,
			//	BES_SAVE_DIR, romname);
			//fprintf(stderr, "temp1: '%s'\n", temp);
			//temp[strlen(temp)-4] = '\0';
			//fprintf(stderr, "temp2: '%s'\n", temp);
			//strcat(temp, ".nst");
			//fprintf(stderr, "temp3: '%s'\n", temp);
			break;
		default:
			snapshotAvailable = 0;
	}
	if (stat(temp.c_str(), &fileinfo))
	{
		fprintf(stderr, "Error stat-ing snapshot (ROM) '%s'\n", temp.c_str());
		snapshotAvailable = 0;
		return;			
	}

	/* Check for a screen snapshot */
	temp = BES_FILE_ROOT_DIR "/" BES_SAVE_DIR "/";
	switch(platform) {
		case PLATFORM_SNES:
			temp += "snes/";
			break;
		case PLATFORM_GBA:
		case PLATFORM_GBC:
			temp += "gb/";
			break;
		case PLATFORM_NES:
			temp += "nes/";
			break;
		default:
			snapshotAvailable = 0;
			return;
	}
	temp += romname;
	temp += ".png";

	if (stat(temp.c_str(), &fileinfo))
	{
		fprintf(stderr, "Error stat-ing snapshot (image): '%s'\n", temp.c_str());
	} else {
		/* Try to load the screen snapshot */
		if (snapshotImage) SDL_FreeSurface(snapshotImage);
		tempSurface = IMG_Load(temp.c_str());
		if (!tempSurface)
		{
			fprintf(stderr, "Error loading snapshot (image)\n");
			return;
		}
		snapshotImage = SDL_ConvertSurface(tempSurface, screen512->format, 0);
		SDL_FreeSurface(tempSurface);

		SDL_LockSurface(snapshotImage);
		SDL_LockSurface(tinySnapshotImage);
		/* Shrink the screen snapshot from disk to the 128x96 size */
		xStep = snapshotImage->w / 128.0;
		yStep = snapshotImage->h / 96.0;
		for (dstRow = 0; dstRow < 96; dstRow++)
		{
			srcPixel = (uint16_t *)snapshotImage->pixels;
			srcPixel += (int)(dstRow * yStep) * 
				(snapshotImage->pitch / 2); 
			dstPixel = (uint16_t *)tinySnapshotImage->pixels;
			dstPixel += (dstRow * tinySnapshotImage->pitch / 2);
		           
			for (dstCol = 0; dstCol < 128; dstCol++)
			{                 
				*dstPixel++ = *(srcPixel + 
					(int)(dstCol * xStep));
			}                 
		}
		SDL_UnlockSurface(tinySnapshotImage);
		SDL_UnlockSurface(snapshotImage);
	}
}


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

static SDL_Surface *pauseGui = NULL;

static uint32_t bgColor;
static const char *headerText = "No ROMs Installed!";
static const char *menuText[6] = { 
  "Welcome to your new Beagle", 
  "Entertainment System! Before",
  "using your system, you must", 
  "install at least one ROM file.",
  "Please refer to the manual for", 
  "ROM installation instructions." }; 
static SDL_Surface *menuImage[7] = { 
	NULL, NULL, NULL, NULL, NULL, NULL, NULL }; 
static SDL_Rect menuTextPos[7] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0},
	{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };

static SDL_Surface *pauseMenuSurface = NULL;
static uint32_t forceUnpause = 0, done = 0;

static void renderPauseGui(void);

void loadNoGamesGui(void) {
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
	menuImage[5] = TTF_RenderText_Blended(fontFSB16, menuText[4],
		textColor);
	menuImage[6] = TTF_RenderText_Blended(fontFSB16, menuText[5],
		textColor);

}

void renderPauseGui(void) {
	SDL_Rect bgRect = {0,0,(PAUSE_GUI_WIDTH - 8), (PAUSE_GUI_HEIGHT-8)};
	SDL_Rect pos = {0,0,0,0};
	uint32_t i;

	menuTextPos[0].x = (PAUSE_GUI_WIDTH / 2) - (menuImage[0]->w / 2); 
	menuTextPos[0].y = 8;
	
	for (i=1; i < 7; i++)
	{
		menuTextPos[i].x = /*19*/25;
		menuTextPos[i].y = 32 + (i * /*36*/24);
	}

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
	for (i=0; i < 7; i++)
		SDL_BlitSurface(menuImage[i], NULL, screenPause, &(menuTextPos[i]));

  /* Finally, now that we have rendered everything, cache a copy of it */
	pos.x = pos.y = 0; 
	pos.w = PAUSE_GUI_WIDTH; pos.h = PAUSE_GUI_HEIGHT;
	pauseMenuSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, PAUSE_GUI_WIDTH,
		PAUSE_GUI_HEIGHT, 16, 0x7C00, 0x03E0, 0x001F, 0);
	SDL_BlitSurface(screenPause, &pos, pauseMenuSurface, NULL); 
}

uint32_t doNoGamesGui(void)
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
	buffer = NULL;

	buffer = calloc(1, 1024*1024*2);
	if (!buffer) {
		fprintf(stderr, "\nERROR: Unable to allocate 1024x1024 texture buffer\n");
		exit(1);
	}
 
	/* Render the pause menu on the screen and make a copy of it */
	renderPauseGui();

	done = 0; forceUnpause = 0;

	/* Do slide-in animation */
	dialogXPos = 1.75f;
	dialogYPos = 0.0f;
	if (guiSize == GUI_NORMAL)
		scaleX = scaleY = 0.5f;
	else 
		scaleX = scaleY = 1.0f;

	SDL_BlitSurface(pauseMenuSurface, NULL, screenPause, &menuPos);
	/* Wait a moment with the black screen */
	EGLBlitGL(buffer);
	EGLFlip();
	usleep(500*1000);
	startAudio();
	playSelectSnd();
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
			returnVal = 1;
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


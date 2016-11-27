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
#include "besKeys.h"
#include "besControls.h"

static SDL_Surface *audioDlg = NULL;

static uint32_t bgColor;
static const char *menuText[4] = {
	"No Audio Detected",
	"There is no audio device available.", 
	"No game audio will be played.", 
	"Continue" }; 
static SDL_Surface *menuImage[4] = { 
	NULL, NULL, NULL, NULL }; 
static SDL_Rect menuTextPos[4] = { {0,0,0,0}, {0,0,0,0}, 
	{0,0,0,0}, {0,0,0,0} };

static SDL_Surface *audioDlgSurface = NULL;
static uint32_t done = 0;
static uint32_t frameCounter = 0, currentIndex = 0, nextIndex = 0;
static SDL_Rect markerPos[3] = { {0,0,0,0},{0,0,0,0},{0,0,0,0} };

static void loadAudioDlg(void) {
	SDL_Surface *tempSurface;
	SDL_PixelFormat *format;

	format = screenPause->format;

	/* Load the graphics for the pause menu */
	tempSurface = IMG_Load("gfx/pausemenu_320x240.png");
	if (!tempSurface)
		fprintf(stderr, "Unable to load Pause GUI images\n");
	audioDlg = SDL_ConvertSurface(tempSurface, format, 0);
	SDL_FreeSurface(tempSurface);

	bgColor = SDL_MapRGB(screenPause->format, 0x28, 0x28, 0x80);

	/* Render menu text */
	menuImage[0] = TTF_RenderText_Blended(fontFSB25, menuText[0],
		textColor);
	menuImage[1] = TTF_RenderText_Blended(fontFSB16, menuText[1],
		textColor);
	menuImage[2] = TTF_RenderText_Blended(fontFSB16, menuText[2],
		textColor);
	menuImage[3] = TTF_RenderText_Blended(fontFS16, menuText[3],
		textColor);
}

static void renderAudioDlg(void) {
	SDL_Rect bgRect = {0,0,(PAUSE_GUI_WIDTH - 8), (PAUSE_GUI_HEIGHT-8)};
	SDL_Rect pos = {0,0,0,0};
	uint32_t i;

	menuTextPos[0].x = (PAUSE_GUI_WIDTH / 2) - (menuImage[0]->w / 2); 
	menuTextPos[0].y = 8;
  
	for (i=1; i < 3; i++)
	{
		menuTextPos[i].x = 15;
		menuTextPos[i].y = 40 + ((i-1) * 22);
	}

	menuTextPos[3].x = 19 + 2 + 100;
	menuTextPos[3].y = 32 + 36 + 68;
	markerPos[0].x = 5 + 100 - 3;
	markerPos[0].y = 30 + 36 + 68;

	bgRect.x = bgRect.y = 4;

	/* Corners */
	pos.x = pos.y = 0;
	SDL_BlitSurface(audioDlg, &(guiPiece[UL_CORNER]), screenPause, &pos);	
	pos.y = PAUSE_GUI_HEIGHT - 8;
	SDL_BlitSurface(audioDlg, &(guiPiece[LL_CORNER]), screenPause, &pos);
	pos.x = PAUSE_GUI_WIDTH - 8; pos.y = 0;
	SDL_BlitSurface(audioDlg, &(guiPiece[UR_CORNER]), screenPause, &pos);
	pos.y = PAUSE_GUI_HEIGHT - 8;
	SDL_BlitSurface(audioDlg, &(guiPiece[LR_CORNER]), screenPause, &pos);

	/* Top */
	pos.y = 0;
	for (i=0; i < 36; i++)
	{
		pos.x = 8 + (i*8);
		SDL_BlitSurface(audioDlg, &guiPiece[TOP_BORDER], screenPause, &pos);
	}

	/* Bottom */
	pos.y = PAUSE_GUI_HEIGHT - 4;
	for (i=0; i < 36; i++)
	{     
		pos.x = 8 + (i*8);
		SDL_BlitSurface(audioDlg, &guiPiece[BOTTOM_BORDER], screenPause, &pos);
	}

	/* Left */
	pos.x = 0;
	for (i=0; i < 26; i++)
	{     
		pos.y = 8 + (i*8);
		SDL_BlitSurface(audioDlg, &guiPiece[LEFT_BORDER], screenPause, &pos);
	}

	/* Right */
	pos.x = PAUSE_GUI_WIDTH - 4;
	for (i=0; i < 26; i++)
	{     
		pos.y = 8 + (i*8);
		SDL_BlitSurface(audioDlg, &guiPiece[RIGHT_BORDER], screenPause, &pos);
	}
	
	/* Background */
	SDL_FillRect(screenPause, &bgRect, bgColor); 

	/* Menu text */
	for (i=0; i < 4; i++)
		SDL_BlitSurface(menuImage[i], NULL, screenPause, &(menuTextPos[i]));

  /* Finally, now that we have rendered everything, cache a copy of it */
	pos.x = pos.y = 0; 
	pos.w = PAUSE_GUI_WIDTH; pos.h = PAUSE_GUI_HEIGHT;
	audioDlgSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, PAUSE_GUI_WIDTH,
		PAUSE_GUI_HEIGHT, 16, 0x7C00, 0x03E0, 0x001F, 0);
	SDL_BlitSurface(screenPause, &pos, audioDlgSurface, NULL); 
}

void doAudioDlg(void)
{
	SDL_Rect menuPos = {0,0,0,0};
	struct timeval startTime, endTime;
	uint64_t elapsedTime;
	SDL_Event event;
	uint8_t i;
	float dialogXPos, dialogYPos, scaleX, scaleY;

	loadAudioDlg();

	BESPauseComboCurrent = 0;

	/* Turn the joystick interface back on */
	BESResetJoysticks();

	/* Render the GBA menu on the screen and make a copy of it */
	renderAudioDlg();

	done = nextIndex = currentIndex = frameCounter = 0;

	/* Do slide-in animation */
	dialogXPos = 1.75f;
	dialogYPos = 0.0f;
	if (guiSize == GUI_NORMAL)
		scaleX = scaleY = 0.5f;
	else
		scaleX = scaleY = 1.0f;

	SDL_BlitSurface(audioDlgSurface, NULL, screenPause, &menuPos);
	for(i = 0; i < 35; i++)
	{
		gettimeofday(&startTime, NULL);
		EGLBlitGBAGL(screenPause->pixels, dialogXPos, dialogYPos, scaleX, scaleY);
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
		SDL_BlitSurface(audioDlgSurface, NULL, screenPause, &menuPos);
		/* Render the marker */
		SDL_BlitSurface(dlgMarker, NULL, screenPause, &(markerPos[nextIndex]));
		EGLBlitGBAGL(screenPause->pixels, dialogXPos, dialogYPos, scaleX, scaleY);
		EGLFlip();

		BESProcessEvents();

		/* Check for events */
		while ( SDL_PollEvent(&event) ) {
			switch (event.type) {
				case SDL_MOUSEBUTTONDOWN:
					break;

				case SDL_JOYBUTTONDOWN:
				case SDL_JOYBUTTONUP:
				//case SDL_JOYAXISMOTION:
					BESProcessJoystickEvent(&event);
					break;

				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case BES_P1_SE:
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
	} /* End while loop */

	/* Slide back out */
  SDL_BlitSurface(audioDlgSurface, NULL, screenPause, &menuPos);
  for(i = 0; i < 45; i++)
  {
    gettimeofday(&startTime, NULL);
    EGLBlitGBAGL(screenPause->pixels, dialogXPos, dialogYPos, scaleX, scaleY);
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
  BESPauseComboCurrent = 0;

  /* Disable joysticks */
  SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}


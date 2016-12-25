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
#include <sys/time.h>
#include <SDL.h>
#include <SDL/SDL_image.h>
#include "gui.h"

static SDL_Surface *logoText = NULL;
static SDL_Surface *beagleSprites = NULL;

static SDL_Rect beagleRect[3] = {
	{0,0,135,165}, {180,0,135,165}, {360,0,135,165} };
static SDL_Rect teleRect = {50, 0, 140, 320};
static SDL_Rect currentPos = {30, 0, 0, 0};
static SDL_Rect logoTextPos = {50, 165, 0, 0};
static void doTele(void);

#define SPLASH_TIME_PER_FRAME (1000000 / 48)

void doSplashScreen(void)
{
	struct timeval startTime, endTime, elapsedTime;

	/* Load the images for the splash screen */
	logoText = IMG_Load("gfx/splash_logo.png");
	beagleSprites = IMG_Load("gfx/splash_720x480_boris.png");

	/* Load the audio for the splash screen */
	loadSplashAudio();

	gettimeofday(&startTime, NULL);
	SDL_BlitSurface(logoText, NULL, screen1024, &logoTextPos);
	EGLBlitGL(screen1024->pixels);
	EGLFlip();
	playCoinSnd();
	
	pthread_join(loadingThread, NULL);
	gettimeofday(&endTime, NULL);
	elapsedTime.tv_sec = endTime.tv_sec - startTime.tv_sec;
	elapsedTime.tv_usec = endTime.tv_usec - endTime.tv_usec;	

	if (elapsedTime.tv_sec < 2)
		usleep(2000000 - (elapsedTime.tv_sec * 1000000) - 
			elapsedTime.tv_usec);

	doTele();
	usleep(1500000);

	if (!audioAvailable)
		doAudioDlg();

	SDL_FreeSurface(logoText);
	SDL_FreeSurface(beagleSprites);
}

static void doTele(void)
{
	uint32_t frameCount = 0;
	struct timeval startTime, endTime;
	int passedTime;
	int yBasePos = 120;
	int yInc = 25;

	playTeleSnd();
	usleep(200000);
	while (frameCount < 15)
	{
		gettimeofday(&startTime, NULL);

		/* Clear teleport area */
		SDL_FillRect(screen1024, &teleRect, 0x0);

		/* Draw shrinking frame */
		if ((frameCount == 0) || (frameCount == 2))
		{
			currentPos.y = yBasePos;
			SDL_BlitSurface(beagleSprites, &beagleRect[1], screen1024, &currentPos); 
		}
		/* Draw down frame */
		else if (frameCount == 1)
		{
			currentPos.y = yBasePos;
			SDL_BlitSurface(beagleSprites, &beagleRect[2], screen1024, &currentPos);
		}
		/* Draw tele up */
		else
		{
			currentPos.y = yBasePos - (frameCount * yInc);
			SDL_BlitSurface(beagleSprites, &beagleRect[0], screen1024, &currentPos);
		}
		EGLBlitGL(screen1024->pixels);
		EGLFlip();
		gettimeofday(&endTime, NULL);
		frameCount++;
		passedTime = (endTime.tv_sec - startTime.tv_sec) * 1000000;
		passedTime += (endTime.tv_usec - startTime.tv_usec);
		if (passedTime < SPLASH_TIME_PER_FRAME)
			usleep(SPLASH_TIME_PER_FRAME - passedTime); 
	}
}

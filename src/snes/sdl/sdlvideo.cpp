/***********************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.

  See CREDITS file to find the copyright owners of this file.

  SDL Input/Audio/Video code (many lines of code come from snes9x & drnoksnes)
  (c) Copyright 2011         Makoto Sugano (makoto.sugano@gmail.com)

  Snes9x homepage: http://www.snes9x.com/

  Permission to use, copy, modify and/or distribute Snes9x in both binary
  and source form, for non-commercial purposes, is hereby granted without
  fee, providing that this license information and copyright notice appear
  with all copies and any derived work.

  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software or it's derivatives.

  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes,
  but is not limited to, charging money for Snes9x or software derived from
  Snes9x, including Snes9x or derivatives in commercial game bundles, and/or
  using Snes9x as a promotion for your commercial product.

  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.

  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
 ***********************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "snes9x.h"
#include "memmap.h"
#include "ppu.h"
#include "controls.h"
#include "movie.h"
#include "logger.h"
#include "conffile.h"
#include "blit.h"
#include "display.h"

#include "sdl_snes9x.h"

#if 1 // AWH - BeagleSNES
#include "gui.h"
extern const char *rom_filename;
#endif // BeagleSNES

struct GUIData
{
	SDL_Surface             *sdl_screen;
	uint8			*snes_buffer;
	uint8			*blit_screen;
	uint32			blit_screen_pitch;
	int			video_mode;
        bool8                   fullscreen;
};
static struct GUIData	GUI;

typedef	void (* Blitter) (uint8 *, int, uint8 *, int, int, int);

#ifdef __linux
// Select seems to be broken in 2.x.x kernels - if a signal interrupts a
// select system call with a zero timeout, the select call is restarted but
// with an infinite timeout! The call will block until data arrives on the
// selected fd(s).
//
// The workaround is to stop the X library calling select in the first
// place! Replace XPending - which polls for data from the X server using
// select - with an ioctl call to poll for data and then only call the blocking
// XNextEvent if data is waiting.
#define SELECT_BROKEN_FOR_SIGNALS
#endif

enum
{
	VIDEOMODE_BLOCKY = 1,
	VIDEOMODE_TV,
	VIDEOMODE_SMOOTH,
	VIDEOMODE_SUPEREAGLE,
	VIDEOMODE_2XSAI,
	VIDEOMODE_SUPER2XSAI,
	VIDEOMODE_EPX,
	VIDEOMODE_HQ2X
};

static void SetupImage (void);
static void TakedownImage (void);
/* AWH static void Repaint (bool8); */

void S9xExtraDisplayUsage (void)
{
	S9xMessage(S9X_INFO, S9X_USAGE, "-fullscreen                     fullscreen mode (without scaling)");
	S9xMessage(S9X_INFO, S9X_USAGE, "");
	S9xMessage(S9X_INFO, S9X_USAGE, "-v1                             Video mode: Blocky (default)");
	S9xMessage(S9X_INFO, S9X_USAGE, "-v2                             Video mode: TV");
	S9xMessage(S9X_INFO, S9X_USAGE, "-v3                             Video mode: Smooth");
	S9xMessage(S9X_INFO, S9X_USAGE, "-v4                             Video mode: SuperEagle");
	S9xMessage(S9X_INFO, S9X_USAGE, "-v5                             Video mode: 2xSaI");
	S9xMessage(S9X_INFO, S9X_USAGE, "-v6                             Video mode: Super2xSaI");
	S9xMessage(S9X_INFO, S9X_USAGE, "-v7                             Video mode: EPX");
	S9xMessage(S9X_INFO, S9X_USAGE, "-v8                             Video mode: hq2x");
	S9xMessage(S9X_INFO, S9X_USAGE, "");
}

void S9xParseDisplayArg (char **argv, int &i, int argc)
{
	if (!strncasecmp(argv[i], "-fullscreen", 11))
        {
                GUI.fullscreen = TRUE;
                printf ("Entering fullscreen mode (without scaling).\n");
        }
        else
	if (!strncasecmp(argv[i], "-v", 2))
	{
		switch (argv[i][2])
		{
			case '1':	GUI.video_mode = VIDEOMODE_BLOCKY;		break;
			case '2':	GUI.video_mode = VIDEOMODE_TV;			break;
			case '3':	GUI.video_mode = VIDEOMODE_SMOOTH;		break;
			case '4':	GUI.video_mode = VIDEOMODE_SUPEREAGLE;	break;
			case '5':	GUI.video_mode = VIDEOMODE_2XSAI;		break;
			case '6':	GUI.video_mode = VIDEOMODE_SUPER2XSAI;	break;
			case '7':	GUI.video_mode = VIDEOMODE_EPX;			break;
			case '8':	GUI.video_mode = VIDEOMODE_HQ2X;		break;
		}
	}
	else
		S9xUsage();
}

const char * S9xParseDisplayConfig (ConfigFile &conf, int pass)
{
	if (pass != 1)
#if 0 // AWH - BeagleSNES
		return ("Unix/SDL");
#else
		return ("BeagleSNES");
#endif // AWH
	if (conf.Exists("Unix/SDL::VideoMode"))
	{
		GUI.video_mode = conf.GetUInt("Unix/SDL::VideoMode", VIDEOMODE_BLOCKY);
		if (GUI.video_mode < 1 || GUI.video_mode > 8)
			GUI.video_mode = VIDEOMODE_BLOCKY;
	}
	else
		GUI.video_mode = VIDEOMODE_BLOCKY;
#if 0 // AWH - BeagleSNES
	return ("Unix/SDL");
#else
	return ("BeagleSNES");
#endif // AWH
}

static void FatalError (const char *str)
{
	fprintf(stderr, "%s\n", str);
	S9xExit();
}

void S9xInitDisplay (int argc, char **argv)
{
#if 0 // AWH - Already initialized in GUI
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
	}
#endif // AWH
	atexit(SDL_Quit);
	
	/*
	 * domaemon
	 *
	 * we just go along with RGB565 for now, nothing else..
	 */
	
	S9xSetRenderPixelFormat(RGB565);
	
	S9xBlitFilterInit();
#if 0 // AWH
	S9xBlit2xSaIFilterInit();
	S9xBlitHQ2xFilterInit();

	/*
	 * domaemon
	 *
	 * FIXME: The secreen size should be flexible
	 * FIXME: Check if the SDL screen is really in RGB565 mode. screen->fmt	
	 */	
        if (GUI.fullscreen == TRUE)
        {
                GUI.sdl_screen = SDL_SetVideoMode(0, 0, 16, SDL_FULLSCREEN);
        } else {
                /* AWH - Modified */
                // NTSC GUI.sdl_screen = SDL_SetVideoMode(512 + 104, 478, 16, 0);
#if defined(CAPE_LCD3)
GUI.sdl_screen = SDL_SetVideoMode(320, 240, 16, 0);
#else
		GUI.sdl_screen = SDL_SetVideoMode(640, 480, 16, 0);
#endif
		//GUI.sdl_screen = SDL_SetVideoMode(SNES_WIDTH, SNES_HEIGHT, 16, 0);
        }

        if (GUI.sdl_screen == NULL)
	{
		printf("Unable to set video mode: %s\n", SDL_GetError());
		exit(1);
        }
fprintf(stderr, "screen %dx%d, pitch: %d\n", GUI.sdl_screen->w, GUI.sdl_screen->h, GUI.sdl_screen->pitch);
	/*
	 * domaemon
	 *
	 * buffer allocation, quite important
	 */
#else
	GUI.sdl_screen = screen512;
#endif // AWH
	SetupImage();
}

void S9xDeinitDisplay (void)
{
	TakedownImage();

	SDL_Quit();

	S9xBlitFilterDeinit();
#if 0 // AWH
	S9xBlit2xSaIFilterDeinit();
	S9xBlitHQ2xFilterDeinit();
#endif // AWH
}

static void TakedownImage (void)
{
	if (GUI.snes_buffer)
	{
		free(GUI.snes_buffer);
		GUI.snes_buffer = NULL;
	}

	S9xGraphicsDeinit();
}

static void SetupImage (void)
{
	TakedownImage();

	// domaemon: The whole unix code basically assumes output=(original * 2);
	// This way the code can handle the SNES filters, which does the 2X.
	GFX.Pitch = SNES_WIDTH * 2 * 2;
	GUI.snes_buffer = (uint8 *) calloc(GFX.Pitch * ((SNES_HEIGHT_EXTENDED + 4) * 2), 1);
	if (!GUI.snes_buffer)
		FatalError("Failed to allocate GUI.snes_buffer.");

	// domaemon: Add 2 lines before drawing.
	GFX.Screen = (uint16 *) (GUI.snes_buffer + (GFX.Pitch * 2 * 2));

	if (GUI.fullscreen == TRUE)
	{
fprintf(stderr, "Using fullscreen mode\n");
		int offset_height_pix;
		int offset_width_pix;
		int offset_byte;

		offset_height_pix = (GUI.sdl_screen->h - (SNES_HEIGHT * 2)) / 2;
		offset_width_pix = (GUI.sdl_screen->w - (SNES_WIDTH * 2)) / 2;
		offset_byte = (GUI.sdl_screen->w * offset_height_pix + offset_width_pix) * 2;

		GUI.blit_screen       = (uint8 *) GUI.sdl_screen->pixels + offset_byte;
		GUI.blit_screen_pitch = GUI.sdl_screen->pitch;
	}
	else 
	{
#if 0 // AWH
		GUI.blit_screen       = (uint8 *) GUI.sdl_screen->pixels;
		GUI.blit_screen_pitch = SNES_WIDTH * 2 * 2; // window size =(*2); 2 byte pir pixel =(*2)
#else
                // NTSC GUI.blit_screen       = (uint8 *) GUI.sdl_screen->pixels + 208 + GUI.sdl_screen->pitch * 14;
#if defined(CAPE_LCD3)
                GUI.blit_screen = (uint8 *) GUI.sdl_screen->pixels + 64 + GUI.sdl_screen->pitch * 7;
#else
		GUI.blit_screen = (uint8 *) GUI.sdl_screen->pixels + 128 + GUI.sdl_screen->pitch * 14;
#endif
                GUI.blit_screen_pitch = GUI.sdl_screen->pitch;
#endif // AWH
	}

	S9xGraphicsInit();
}

void S9xPutImage (int width, int height)
{
	static int	prevWidth = 0, prevHeight = 0;
#if 0 /* AWH */
	int			copyWidth, copyHeight;
	Blitter		blitFn = NULL;

	if (GUI.video_mode == VIDEOMODE_BLOCKY || GUI.video_mode == VIDEOMODE_TV || GUI.video_mode == VIDEOMODE_SMOOTH)
		if ((width <= SNES_WIDTH) && ((prevWidth != width) || (prevHeight != height)))
			S9xBlitClearDelta();

	if (width <= SNES_WIDTH)
	{
		if (height > SNES_HEIGHT_EXTENDED)
		{
			copyWidth  = width * 2;
			copyHeight = height;
			blitFn = S9xBlitPixSimple2x1;
		}
		else
		{
			copyWidth  = width  * 2;
			copyHeight = height * 2;

			switch (GUI.video_mode)
			{
				case VIDEOMODE_BLOCKY:		blitFn = S9xBlitPixSimple2x2;		break;
				case VIDEOMODE_TV:			blitFn = S9xBlitPixTV2x2;			break;
				case VIDEOMODE_SMOOTH:		blitFn = S9xBlitPixSmooth2x2;		break;
				case VIDEOMODE_SUPEREAGLE:	blitFn = S9xBlitPixSuperEagle16;	break;
				case VIDEOMODE_2XSAI:		blitFn = S9xBlitPix2xSaI16;			break;
				case VIDEOMODE_SUPER2XSAI:	blitFn = S9xBlitPixSuper2xSaI16;	break;
				case VIDEOMODE_EPX:			blitFn = S9xBlitPixEPX16;			break;
				case VIDEOMODE_HQ2X:		blitFn = S9xBlitPixHQ2x16;			break;
			}
		}
	}
	else
	if (height <= SNES_HEIGHT_EXTENDED)
	{
		copyWidth  = width;
		copyHeight = height * 2;

		switch (GUI.video_mode)
		{
			default:					blitFn = S9xBlitPixSimple1x2;	break;
			case VIDEOMODE_TV:			blitFn = S9xBlitPixTV1x2;		break;
		}
	}
	else
	{
		copyWidth  = width;
		copyHeight = height;
		blitFn = S9xBlitPixSimple1x1;
	}


	// domaemon: this is place where the rendering buffer size should be changed?
	blitFn((uint8 *) GFX.Screen, GFX.Pitch, GUI.blit_screen, GUI.blit_screen_pitch, width, height);

	// domaemon: does the height change on the fly?
	if (height < prevHeight)
	{
		int	p = GUI.blit_screen_pitch >> 2;
		for (int y = SNES_HEIGHT * 2; y < SNES_HEIGHT_EXTENDED * 2; y++)
		{
			uint32	*d = (uint32 *) (GUI.blit_screen + y * GUI.blit_screen_pitch);
			for (int x = 0; x < p; x++)
				*d++ = 0;
		}
	}
	Repaint(TRUE);

	prevWidth  = width;
	prevHeight = height;
#else /* AWH */
	if ((prevWidth != width) || (prevHeight != height))
	{
		if ((width <= 256) && (height <= 256))
		{
			GUI.blit_screen = (uint8 *)screen256->pixels;
			GUI.blit_screen_pitch = screen256->pitch;
		} else {
			GUI.blit_screen = (uint8 *)screen512->pixels;
			GUI.blit_screen_pitch = screen512->pitch;
		}
		prevWidth = width; prevHeight = height;
		EGLSrcSize(width, height);
	}

	S9xBlitPixSimple1x1((uint8 *) GFX.Screen, GFX.Pitch, GUI.blit_screen, 
		GUI.blit_screen_pitch, width, height);

	switch(BESPauseState)
	{
		case PAUSE_CACHE:
		case PAUSE_CACHE_NO_DRAW:
			EGLBlitGLCache(GUI.blit_screen, 1);
			if (BESPauseState == PAUSE_CACHE)
				EGLFlip();
			Settings.Paused = 1;
			if (doPauseGui(rom_filename, PLATFORM_SNES))
				exit(0);
			Settings.Paused = 0;
			BESPauseState = PAUSE_NONE;
			break;

		case PAUSE_NONE:
			EGLBlitGL(GUI.blit_screen);
			EGLFlip();
			break;
	}
#endif // AWH
}

#if 0 /* AWH */
static void Repaint (bool8 isFrameBoundry)
{
if (GUI.fullscreen == TRUE)
        SDL_Flip(GUI.sdl_screen);
#if 1 // AWH - BeagleSNES
else
        //SDL_UpdateRect(GUI.sdl_screen, /*(GUI.sdl_screen->w - (SNES_WIDTH << 1)) >> 1*/ 104 , /*(GUI.sdl_screen->h - (SNES_HEIGHT << 1)) >> 1*/ 34, /*SNES_WIDTH * 2*/512, /*SNES_HEIGHT * 2*/448);
SDL_UpdateRect(GUI.sdl_screen, 104, 6, 512, 472 /* 478 - 6 */);
#endif // BeagleSNES
}
#endif /* AWH */

void S9xMessage (int type, int number, const char *message)
{
	const int	max = 36 * 3;
	static char	buffer[max + 1];

	fprintf(stdout, "%s\n", message);
	strncpy(buffer, message, max + 1);
	buffer[max] = 0;
	S9xSetInfoString(buffer);
}

const char * S9xStringInput (const char *message)
{
	static char	buffer[256];

	printf("%s: ", message);
	fflush(stdout);

	if (fgets(buffer, sizeof(buffer) - 2, stdin))
		return (buffer);

	return (NULL);
}

void S9xSetTitle (const char *string)
{
	SDL_WM_SetCaption(string, string);
}

void S9xSetPalette (void)
{
	return;
}

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

#include <SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include "gui.h"

#if defined(BEAGLEBONE_BLACK)
#define INSTRUCT_X_POS 80
#else
#define INSTRUCT_X_POS 55
#endif
#define INSTRUCT_Y_POS 375

static SDL_Surface *instructDpad;
static SDL_Surface *instructDpadText;
static SDL_Surface *instructButton;
static SDL_Surface *instructButtonText;
static SDL_Surface *instructText[2];

static SDL_Color instructTextColor={155,152,152,255};

static SDL_Rect instructDpadRect = {INSTRUCT_X_POS, INSTRUCT_Y_POS, 0, 0};
static SDL_Rect instructButtonRect = {INSTRUCT_X_POS + 70, INSTRUCT_Y_POS + 45, 0, 0};
static SDL_Rect instructDpadTextRect = {INSTRUCT_X_POS + 50, INSTRUCT_Y_POS, 0, 0};
static SDL_Rect instructButtonTextRect = {INSTRUCT_X_POS, INSTRUCT_Y_POS + 45, 0, 0};
static SDL_Rect instructTextRect[2] = { {INSTRUCT_X_POS-25, INSTRUCT_Y_POS, 0, 0},
  {INSTRUCT_X_POS-25, INSTRUCT_Y_POS + 35, 0, 0} };

void loadInstruct(void)
{
  instructDpad = IMG_Load("gfx/dpad.png");
  instructButton = IMG_Load("gfx/button.png");

  //instructFont = TTF_OpenFont("fonts/FreeSans.ttf", 24);
  instructDpadText = TTF_RenderText_Blended(fontFS24, "to select game", instructTextColor);
  instructButtonText = TTF_RenderText_Blended(fontFS24, "Press       to Start", instructTextColor);
  instructText[0] = TTF_RenderText_Blended(fontFS24, "PLEASE CONNECT", instructTextColor);
  instructText[1] = TTF_RenderText_Blended(fontFS24, "PLAYER 1 GAMEPAD", instructTextColor);
}

void renderInstruct(SDL_Surface *screen, const uint32_t gamepadPresent) 
{
  if (gamepadPresent) {
    SDL_BlitSurface(instructDpad, NULL, screen, &instructDpadRect);
    SDL_BlitSurface(instructDpadText, NULL, screen, &instructDpadTextRect);
    SDL_BlitSurface(instructButton, NULL, screen, &instructButtonRect);
    SDL_BlitSurface(instructButtonText, NULL, screen, &instructButtonTextRect);
  } else {
    SDL_BlitSurface(instructText[0], NULL, screen, &instructTextRect[0]);
    SDL_BlitSurface(instructText[1], NULL, screen, &instructTextRect[1]);
  }
}


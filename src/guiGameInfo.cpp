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

#define X_POS 350
#define Y_POS 80
#define NUM_TEXT_LINES 5

typedef struct {
  SDL_Surface *box;
  SDL_Surface *itemText[NUM_TEXT_LINES];
  SDL_Surface *genreText;
  SDL_Surface *dateText;
  SDL_Surface *titleText;
} infoPanel_t;

static infoPanel_t *infoPanel;
static SDL_Surface *genreHeader;
static SDL_Surface *dateHeader;

static SDL_Color itemTextColor={255,255,255,255};
static SDL_Color headerTextColor={192,192,192,255};

static SDL_Rect titleDstRect={X_POS + 18, Y_POS + 5, 0, 0};
static SDL_Rect backgroundRect={X_POS, Y_POS, 720-X_POS-1, 480-Y_POS};
static SDL_Rect boxDstRect={X_POS + 15, Y_POS + 50, 0, 0};
static SDL_Rect genreHeaderDstRect={X_POS + 15, Y_POS + 350, 0, 0};
static SDL_Rect genreTextDstRect={X_POS + 130, Y_POS + 348, 0, 0};
static SDL_Rect dateHeaderDstRect={X_POS + 15, Y_POS + 330, 0, 0};
static SDL_Rect dateTextDstRect={X_POS + 130, Y_POS + 328, 0, 0};
static SDL_Rect textDstRect = {X_POS + 15, Y_POS + 210, 0, 0};
static SDL_Rect tempRect;

void loadGameInfo(void)
{
  int i = 0;
  std::string tempBuf;

  genreHeader = TTF_RenderText_Blended(fontFSB12, "GENRE:", headerTextColor);
  dateHeader = TTF_RenderText_Blended(fontFSB12, "RELEASE DATE:", headerTextColor);

  infoPanel = (infoPanel_t *)calloc(vGameInfo.size(), sizeof(infoPanel_t));

  /* Dynamic loading of game information */
  for (std::vector<gameInfo_t>::iterator it = vGameInfo.begin(); it != vGameInfo.end(); ++it) {
    /* Move to the next node and render its text */
    infoPanel[i].titleText = TTF_RenderText_Blended(fontFS30, it->gameTitle.c_str(), itemTextColor);

    /* Load the box image */
    if (it->imageFile[0]) {
      tempBuf = std::string(BES_FILE_ROOT_DIR "/images/") + it->imageFile;
    } else
      tempBuf = "gfx/blank_box.png";
    //fprintf(stderr, "Image: '%s'\n", tempBuf.c_str());
    infoPanel[i].box = IMG_Load(tempBuf.c_str());

    /* Render the genre text */
    if (!it->genreText[0].empty()) {
      tempBuf = it->genreText[0];
      if (!it->genreText[1].empty()) {
        tempBuf += "/";
        tempBuf += it->genreText[1];
      }
    } else
      tempBuf = "None Listed";     
    infoPanel[i].genreText = TTF_RenderText_Blended(fontFS16, tempBuf.c_str(), itemTextColor);

    /* Render the release date text */
    infoPanel[i].dateText = TTF_RenderText_Blended(fontFS16, it->dateText.c_str(), itemTextColor);

    /* Render the description text */
    if ( it->infoText[0].empty() &&
      it->infoText[1].empty() &&
      it->infoText[2].empty() &&
      it->infoText[3].empty() &&
      it->infoText[4].empty() )
      infoPanel[i].itemText[0] = TTF_RenderText_Blended(fontFS16, "No description available.", itemTextColor);
    else
    {
      infoPanel[i].itemText[0] = TTF_RenderText_Blended(fontFS16, it->infoText[0].c_str(), itemTextColor);
      infoPanel[i].itemText[1] = TTF_RenderText_Blended(fontFS16, it->infoText[1].c_str(), itemTextColor);
      infoPanel[i].itemText[2] = TTF_RenderText_Blended(fontFS16, it->infoText[2].c_str(), itemTextColor);
      infoPanel[i].itemText[3] = TTF_RenderText_Blended(fontFS16, it->infoText[3].c_str(), itemTextColor);
      infoPanel[i].itemText[4] = TTF_RenderText_Blended(fontFS16, it->infoText[4].c_str(), itemTextColor);
    } /* Description if-else */
    i++;
  } /* Node for loop */
}

void renderGameInfo(SDL_Surface *screen, const uint32_t i) 
{
  int x = 0;
  SDL_FillRect(screen, &backgroundRect, 0x18E3);
  SDL_BlitSurface(infoPanel[i].titleText, NULL, screen, &titleDstRect);
  SDL_BlitSurface(infoPanel[i].box, NULL, screen, &boxDstRect); 
  SDL_BlitSurface(genreHeader, NULL, screen, &genreHeaderDstRect);
  SDL_BlitSurface(infoPanel[i].genreText, NULL, screen, &genreTextDstRect);
  SDL_BlitSurface(dateHeader, NULL, screen, &dateHeaderDstRect);
  SDL_BlitSurface(infoPanel[i].dateText, NULL, screen, &dateTextDstRect);

  tempRect.x = textDstRect.x;
  tempRect.y = textDstRect.y;
  for (x=0; x < NUM_TEXT_LINES; x++) {
    tempRect.y = textDstRect.y + (20 * x); 
    SDL_BlitSurface(infoPanel[i].itemText[x], NULL, screen, &tempRect);
  }
}


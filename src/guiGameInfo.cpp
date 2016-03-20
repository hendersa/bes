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
#define TINY_WIDTH 200
#define TINY_HEIGHT 145

typedef struct {
  SDL_Surface *box;
  SDL_Surface *itemText[NUM_TEXT_LINES];
  SDL_Surface *genreText;
  SDL_Surface *dateText;
  SDL_Surface *titleText;
} infoPanel_t;

static infoPanel_t infoPanel;
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
uint32_t loadedIndex = 1;
static SDL_Surface *tinySnapshotImage = NULL;

void initGameInfo(void)
{
  memset(&infoPanel, 0, sizeof(infoPanel));
  genreHeader = TTF_RenderText_Blended(fontFSB12, "GENRE:", headerTextColor);
  dateHeader = TTF_RenderText_Blended(fontFSB12, "RELEASE DATE:", headerTextColor);
}

static void loadGameInfo(const uint32_t index)
{
  uint8_t i = 0;
  bool empty = true;
  std::string tempBuf;

  loadedIndex = index;

  /* Clear out the infoPanel surfaces */
  if (infoPanel.box) SDL_FreeSurface(infoPanel.box);
  if (infoPanel.genreText) SDL_FreeSurface(infoPanel.genreText);
  if (infoPanel.dateText) SDL_FreeSurface(infoPanel.dateText);
  if(infoPanel.titleText) SDL_FreeSurface(infoPanel.titleText);
  for (i=0; i < NUM_TEXT_LINES; i++)
    if (infoPanel.itemText[i]) SDL_FreeSurface(infoPanel.itemText[i]);
  memset(&infoPanel, 0, sizeof (infoPanel));  

  /* Check to make sure the index is valid */
  if (index >= vGameInfo.size()) {
    fprintf(stderr, "ERROR: loadGameInfo() failed with index %u\n", index);
    return;
  }

  /* Render title text */
  infoPanel.titleText = TTF_RenderText_Blended(fontFS30, vGameInfo[index].gameTitle.c_str(), itemTextColor);

  /* Load the box image */
  if (tinySnapshotImage) {
    SDL_FreeSurface(tinySnapshotImage);
    tinySnapshotImage = NULL;
  }
  tinySnapshotImage = SDL_CreateRGBSurface(SDL_SWSURFACE, TINY_WIDTH,
    TINY_HEIGHT, 16, 0xFC00, 0x07E0, 0x001F, 0);
  if (checkForSnapshot(vGameInfo[index].romFile.c_str(), 
    vGameInfo[index].platform, tinySnapshotImage, TINY_WIDTH, TINY_HEIGHT)) {
      infoPanel.box = tinySnapshotImage;
      tinySnapshotImage = NULL;
  } else {
    //infoPanel.box = IMG_Load("gfx/blank_box.png");
    infoPanel.box = IMG_Load("gfx/blank_snapshot_large.png");
    SDL_FreeSurface(tinySnapshotImage);
    tinySnapshotImage = NULL;
  }

  /* Render the genre text */
  if (!vGameInfo[index].genreText[0].empty()) {
    tempBuf = vGameInfo[index].genreText[0];
    if (!vGameInfo[index].genreText[1].empty()) {
      tempBuf += "/";
      tempBuf += vGameInfo[index].genreText[1];
    }
  } else
    tempBuf = "None Listed";     
  infoPanel.genreText = TTF_RenderText_Blended(fontFS16, tempBuf.c_str(), itemTextColor);

  /* Render the release date text */
  if (!vGameInfo[index].dateText.empty())
    infoPanel.dateText = TTF_RenderText_Blended(fontFS16, vGameInfo[index].dateText.c_str(), itemTextColor);
  else
    infoPanel.dateText = TTF_RenderText_Blended(fontFS16, DEFAULT_DATE_TEXT, itemTextColor);

  /* Render the description text */
  for (i = 0; i < NUM_TEXT_LINES; i++)
    if (!vGameInfo[index].infoText[i].empty()) empty = false;
  if (empty)
    infoPanel.itemText[0] = TTF_RenderText_Blended(fontFS16, "No description available.", itemTextColor);
  else {
    for (i=0; i < NUM_TEXT_LINES; i++)
      infoPanel.itemText[i] = TTF_RenderText_Blended(fontFS16, vGameInfo[index].infoText[i].c_str(), itemTextColor);
  } /* Description if-else */

}

void renderGameInfo(SDL_Surface *screen, const uint32_t index, const bool force) 
{
  uint8_t i = 0;

  if ((index != loadedIndex) || force)
    loadGameInfo(index);

  SDL_FillRect(screen, &backgroundRect, 0x18E3);
  if (infoPanel.titleText)
    SDL_BlitSurface(infoPanel.titleText, NULL, screen, &titleDstRect);
  if (infoPanel.box)
    SDL_BlitSurface(infoPanel.box, NULL, screen, &boxDstRect); 
  SDL_BlitSurface(genreHeader, NULL, screen, &genreHeaderDstRect);
  if (infoPanel.genreText)
    SDL_BlitSurface(infoPanel.genreText, NULL, screen, &genreTextDstRect);
  SDL_BlitSurface(dateHeader, NULL, screen, &dateHeaderDstRect);
  if (infoPanel.dateText)
    SDL_BlitSurface(infoPanel.dateText, NULL, screen, &dateTextDstRect);

  tempRect.x = textDstRect.x;
  tempRect.y = textDstRect.y;
  for (i=0; i < NUM_TEXT_LINES; i++) {
    if (infoPanel.itemText[i]) {
      tempRect.y = textDstRect.y + (20 * i);
      SDL_BlitSurface(infoPanel.itemText[i], NULL, screen, &tempRect);
    }
  }
}


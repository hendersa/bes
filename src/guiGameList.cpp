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

/* Position of the game list on the screen */
#define X_POS 40
#define Y_POS 95
#define MAX_GAMES_PER_SCREEN 6
#define TRACK_HEIGHT 232
#define BASE_SHIFT_FRAME_FACTOR 0.1f

static SDL_Surface *selectOverlay = NULL;
static SDL_Surface **itemText;
static SDL_Surface *headerText;

/* Console icons are SNES, NES, GBA, and GBC, in that order */
static SDL_Surface *consoleIcons = NULL;
static /*const*/ SDL_Rect consoleIconRect[4] = 
  {{0,0,32,32}, {64,0,32,32}, {32,0,32,32}, {96,0,32,32}};

static TTF_Font *itemListFont;
static TTF_Font *headerTextFont;
static /*const*/ SDL_Rect headerTextDstRect = {X_POS+30, Y_POS, 0, 0};
static const SDL_Color itemTextColor={255,255,255,255};
static const SDL_Color headerTextColor = {155,152,152,255};
static SDL_Rect overlay1DstRect={X_POS - 40, 36, 0, 0};
static SDL_Rect itemTextDstRect={X_POS - 16/* ICONS +30*/, 0, 0, 0};
static SDL_Rect lineRect={X_POS - 40, 0, 340, 1};
static SDL_Rect trackRect={X_POS - 40 + 325, Y_POS + 37, 1, TRACK_HEIGHT}; 
static /*const*/ SDL_Rect backgroundRect = {0, 80, 350, 480-80};

/* Game list indices */
static uint32_t currentIndex = 0; /* Selected index in the list */
static uint32_t topIndex = 0; /* Index in list in top slot of game list */
static uint32_t nextIndex = 0; /* Index in list we're moving to during anim */

/* When going to a new index during animation, these control the move speed */
static float shiftFrame = 0.0f, shiftFrameFactor = BASE_SHIFT_FRAME_FACTOR;

static SDL_Surface *itemListSurface;
static SDL_Rect itemListSurfaceRect = {12, 0, 0, 0};
static SDL_Rect itemSurfaceRect = {0, 0, 0, 0};
static SDL_Surface *thumbOrig = NULL;
static SDL_Surface *thumb = NULL;
static SDL_Rect thumbDstRect = {0, 0, 0, 0};

static void renderSingleTextItem(const uint8_t slot, const uint32_t gameIndex)
{
  std::string buffer;
      
  buffer = "      ";
  buffer += vGameInfo[gameIndex].gameTitle;
  itemText[slot] = TTF_RenderText_Blended(itemListFont, buffer.c_str(), itemTextColor);
  SDL_BlitSurface(consoleIcons, &consoleIconRect[vGameInfo[gameIndex].platform - TAG_FIRST_PLATFORM], itemText[slot], NULL);
}

static void renderItemListSurface(void)
{
  uint8_t x;

  itemSurfaceRect.w = itemListSurface->w;
  itemSurfaceRect.h = (selectOverlay->h - 8);

  for (x = 0; x < MAX_GAMES_PER_SCREEN + 2; x++)
  {
    itemListSurfaceRect.y = x * (selectOverlay->h - 8);
    itemSurfaceRect.y = itemListSurfaceRect.y;
    SDL_FillRect(itemListSurface, &itemSurfaceRect, 0x0);
    if (itemText[x] != NULL)
      SDL_BlitSurface(itemText[x], NULL, itemListSurface, &itemListSurfaceRect);
  }
}

static void renderTextItems(const bool shiftDown, const bool warp)
{
  std::string buffer;
  uint8_t gamesToRender = MAX_GAMES_PER_SCREEN;
  uint32_t x = 0;

  /* Here is how this works:
     Rather than pre-render every single game title in its own SDL_Surface and
     cache them (which eats up more and more memory as you add games), I have 
     a pool of 8 entries for the game list pane (6 max games per screen plus 
     one offscreen at the top and bottom for scrolling in): 

     Offscreen  Game A  <-- itemText[0]
     Onscreen   Game B        .
     Onscreen   Game C        .
     Onscreen   Game D        .
     Onscreen   Game E        .
     Onscreen   Game F        .
     Onscreen   Game G        .
     Offscreen  Game H  <-- itemText[7]

     As we scroll up and down, we just shift all entries up or down one,
     discard the one that falls off the list, and render a new one to fill
     the empty slot. When you are at the start of the list, itemText[0] is 
     null (and offscreen), and itemText[1] holds the first game in the list.
     Likewise, when you are at the end of the list, itemText[6] holds the
     last item and itemText[7] is null. This scheme saves us the overhead 
     of re-rendering the text on each frame.
 
     When the menu "warps" to a new location (moves without the overlay
     scrolling), we free ALL of the itemText surfaces and rerender them. 
  */

  if (warp) {
    /* Figure out how many entries to render */
    if ((vGameInfo.size() - topIndex) < MAX_GAMES_PER_SCREEN)
      gamesToRender = vGameInfo.size() - topIndex;

    /* Clear out all of the previous textItems */
    for (x=0; x < (MAX_GAMES_PER_SCREEN + 2); x++)
    {
      if (itemText[x]) {
        SDL_FreeSurface(itemText[x]);
        itemText[x] = NULL;
      }
    }

    /* Rendering the top offscreen entry? */
    if (topIndex > 0)
      renderSingleTextItem(0, (topIndex - 1));

    /* Render the visible textItems */
    for (x = 0; x < gamesToRender; x++)
      renderSingleTextItem((x+1), (x + topIndex));

    /* Rendering the bottom offscreen entry? */
    if ( (vGameInfo.size() > MAX_GAMES_PER_SCREEN) && (topIndex < (vGameInfo.size() - MAX_GAMES_PER_SCREEN)) )
      renderSingleTextItem((MAX_GAMES_PER_SCREEN + 1), (topIndex + MAX_GAMES_PER_SCREEN));

  } /* End "warp" case */
  else if (shiftDown) { /* Are we moving down the list? */
    /* No adjustments needed when the screen can't scroll */
    if (vGameInfo.size() <= MAX_GAMES_PER_SCREEN) return;

    /* No adjustments needed when the screen isn't scrolling */
    if ( ((currentIndex == (topIndex+(MAX_GAMES_PER_SCREEN-1))) 
      && (nextIndex > currentIndex)) ) {

      /* Shift up the textItems */
      if (itemText[0])
        SDL_FreeSurface(itemText[0]);
      for (x = 0; x < (MAX_GAMES_PER_SCREEN + 1); x++)
      {
        itemText[x] = itemText[x+1];
        //renderSingleTextItem(x, topIndex + x);
      }
      itemText[MAX_GAMES_PER_SCREEN + 1] = NULL;

      /* Rendering the bottom offscreen entry? */
      if (topIndex < (vGameInfo.size() - MAX_GAMES_PER_SCREEN - 1))
        renderSingleTextItem((MAX_GAMES_PER_SCREEN + 1), (topIndex + MAX_GAMES_PER_SCREEN + 1));
      
    } /* Check if screen is scrolling */
  } /* End shiftDown case */
  else { /* We're going up the list */
    /* No adjustments needed when the screen can't scroll */
    if (vGameInfo.size() <= MAX_GAMES_PER_SCREEN) return;

    /* No adjustments needed when the screen isn't scrolling */
    if ((currentIndex == topIndex) && (nextIndex < currentIndex))
    {
      /* Shift down the textItems */
      if (itemText[MAX_GAMES_PER_SCREEN + 1])
        SDL_FreeSurface(itemText[MAX_GAMES_PER_SCREEN + 1]);
      for (x = (MAX_GAMES_PER_SCREEN + 1); x > 0; x--)
        itemText[x] = itemText[x-1];

      /* Rendering the top offscreen entry? */
      itemText[0] = NULL;
      if (topIndex > 1)
        renderSingleTextItem(0, (topIndex - 2));

    } /* Check if screen is scrolling */
  } /* End !shiftDown case */

  /* Render all of those textItem[] entries onto the itemListSurface */
  renderItemListSurface();
}

void loadGameLists(void)
{
  int x = 0, thumbHeight = 0;
  SDL_Rect thumbSrcRect = {0, 0, 0, 0};
  std::string buffer;

  /* Load our fonts and gfx */
  itemListFont = TTF_OpenFont("fonts/FreeSans.ttf", 25);
  headerTextFont = TTF_OpenFont("fonts/FreeSansBold.ttf", 25);
  headerText = TTF_RenderText_Blended(headerTextFont, "Game List", headerTextColor);
  selectOverlay = IMG_Load("gfx/overlay_bar.png");
  itemListSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, 290, 8*(selectOverlay->h - 8), 16, 0x7C00, 0x03E0, 0x001F, 0);
  thumbOrig = IMG_Load("gfx/thumb.png");
  consoleIcons = IMG_Load("gfx/icons.png");

  /* Begin constructing the gfx for the games list */
  thumb = NULL;
  itemText = (SDL_Surface **)calloc(MAX_GAMES_PER_SCREEN+2, sizeof(SDL_Surface *));
 
  renderTextItems(false, true);

  /* Do we need to create a thumb for this list? */
  if (vGameInfo.size() > MAX_GAMES_PER_SCREEN)
  {
    /* Figure out how tall the thumb must be */
    thumbHeight = (TRACK_HEIGHT - 4) - ((vGameInfo.size() - MAX_GAMES_PER_SCREEN) / (float)vGameInfo.size()) * (TRACK_HEIGHT - 4);
    if (thumbHeight < 14) thumbHeight = 14;
    thumb = SDL_CreateRGBSurface(SDL_SWSURFACE, thumbOrig->w, thumbHeight, 16, 0x7C00, 0x03E0, 0x001F, 0);
    /* Top of thumb */
    SDL_BlitSurface(thumbOrig, NULL, thumb, NULL);
    /* Bottom of thumb */ 
    thumbSrcRect.w = thumbOrig->w;
    thumbSrcRect.h = 7;
    thumbSrcRect.y = 8;
    thumbDstRect.y = thumbHeight - thumbSrcRect.h;
    SDL_BlitSurface(thumbOrig, &thumbSrcRect, thumb, &thumbDstRect);
    /* Thumb slices */
    thumbSrcRect.y = 7;
    thumbSrcRect.h = 1;
    for (x=0; x<(thumbHeight-14); x++) {
      thumbDstRect.y = x+7;
      SDL_BlitSurface(thumbOrig, &thumbSrcRect, thumb, &thumbDstRect);
    }
    thumbDstRect.x = trackRect.x - 7;
    thumbDstRect.y = trackRect.y;
    SDL_SetColorKey(thumb, SDL_SRCCOLORKEY, 0x7C1F);
  }
}

void renderThumb(SDL_Surface *screen) {
  thumbDstRect.y = trackRect.y + 2 + ((topIndex / (float) (vGameInfo.size()-MAX_GAMES_PER_SCREEN)) * (TRACK_HEIGHT - 4 - thumb->h));

  // If the text list isn't scrolling...
  if ( (nextIndex == currentIndex) ||
    ((nextIndex >= topIndex) && (nextIndex < currentIndex)) ||
    ((nextIndex <= (topIndex + (MAX_GAMES_PER_SCREEN-1))) && (nextIndex > currentIndex)))
  {
    // Nothing
  // Text is scrolling up...
  } else if (nextIndex > currentIndex) {
    thumbDstRect.y += (((shiftFrame / (float)(selectOverlay->h - 8)) / (float) (vGameInfo.size()-MAX_GAMES_PER_SCREEN)) * (TRACK_HEIGHT - 4 - thumb->h));
  // Text is scrolling down...
  } else if (nextIndex < currentIndex) {
    thumbDstRect.y -= (((shiftFrame / (float)(selectOverlay->h - 8)) / (float) (vGameInfo.size()-MAX_GAMES_PER_SCREEN)) * (TRACK_HEIGHT - 4 - thumb->h));
  }
  SDL_BlitSurface(thumb, NULL, screen, &thumbDstRect);
}

void renderGameList(SDL_Surface *screen) {
  uint32_t i; 
  int vertical_offset;
  SDL_Rect clip_rect = {0, 0, 0, itemTextDstRect.h};

  clip_rect.y = (selectOverlay->h - 8);
  clip_rect.w = itemListSurface->w;
  clip_rect.h = (selectOverlay->h - 8) * MAX_GAMES_PER_SCREEN;
  itemListSurfaceRect.y = (Y_POS + 60-15);

  SDL_FillRect(screen, &backgroundRect, 0x0);
  if (guiSize != GUI_SMALL)
  {
    SDL_BlitSurface(headerText, NULL, screen, &headerTextDstRect);
    lineRect.y = Y_POS + 35;
    SDL_FillRect(screen, &lineRect, 0xFFFF);
    lineRect.y = Y_POS + 270;
    SDL_FillRect(screen, &lineRect, 0xFFFF);
  }

  // First case: all games fit on a single screen
  if (vGameInfo.size() <= MAX_GAMES_PER_SCREEN) {
    vertical_offset = (MAX_GAMES_PER_SCREEN-vGameInfo.size()) * (selectOverlay->h - 8) / 2;
    for (i=0; i < vGameInfo.size(); i++) {
      itemTextDstRect.y = (Y_POS + 45 + vertical_offset) + i*(selectOverlay->h - 8);
      SDL_BlitSurface(itemText[i+1], NULL, screen, &itemTextDstRect);
    }
    overlay1DstRect.y = (Y_POS + 40 + vertical_offset) + currentIndex*(selectOverlay->h - 8);

    /* Are we shifting down? */
    if (currentIndex < nextIndex)
      overlay1DstRect.y += shiftFrame;
    /* Are we shifting up? */
    if (currentIndex > nextIndex)
      overlay1DstRect.y -= shiftFrame;

    SDL_BlitSurface(selectOverlay, NULL, screen, &overlay1DstRect);
  }
  // Second case: > MAX_GAMES_PER_SCREEN titles, nothing is moving
  else if (nextIndex == currentIndex) {
    SDL_BlitSurface(itemListSurface, &clip_rect, screen, &itemListSurfaceRect);
    overlay1DstRect.y = (Y_POS + 40) + (currentIndex - topIndex)*(selectOverlay->h - 8);
    SDL_BlitSurface(selectOverlay, NULL, screen, &overlay1DstRect);
    renderThumb(screen);
  }
  // Third case: > MAX_GAMES_PER_SCREEN titles, highlight bar moves
  else if 
    (((nextIndex >= topIndex) && (nextIndex < currentIndex)) ||
    ((nextIndex <= (topIndex + (MAX_GAMES_PER_SCREEN-1))) && (nextIndex > currentIndex)))  
  {
    SDL_BlitSurface(itemListSurface, &clip_rect, screen, &itemListSurfaceRect);
    overlay1DstRect.y = (Y_POS + 40) + (currentIndex-topIndex)*(selectOverlay->h - 8);

    /* Are we shifting down? */
    if (currentIndex < nextIndex)
      overlay1DstRect.y += shiftFrame;
    /* Are we shifting up? */
    if (currentIndex > nextIndex)
      overlay1DstRect.y -= shiftFrame;

    SDL_BlitSurface(selectOverlay, NULL, screen, &overlay1DstRect);
    renderThumb(screen);
  }
  // Fourth case: > MAX_GAMES_PER_SCREEN titles, text scrolls down
  else if ((currentIndex == topIndex) && (nextIndex < currentIndex)) {
    clip_rect.y = (2 * (selectOverlay->h - 8)) - shiftFrame;
    itemListSurfaceRect.y = (Y_POS + 60-15);
    SDL_BlitSurface(itemListSurface, &clip_rect, screen, &itemListSurfaceRect);

    overlay1DstRect.y = (Y_POS + 55-15);
    SDL_BlitSurface(selectOverlay, NULL, screen, &overlay1DstRect);
    renderThumb(screen);
  }
  // Fifth case: > MAX_GAMES_PER_SCREEN titles, text scrolls up
  else if ((currentIndex == (topIndex+(MAX_GAMES_PER_SCREEN-1))) && (nextIndex > currentIndex)) {
    clip_rect.y = shiftFrame; //+ (selectOverlay->h - 8);
    itemListSurfaceRect.y = (Y_POS + 45);
    SDL_BlitSurface(itemListSurface, &clip_rect, screen, &itemListSurfaceRect);
    overlay1DstRect.y = (Y_POS + 40) + (MAX_GAMES_PER_SCREEN-1)*(selectOverlay->h - 8);
    SDL_BlitSurface(selectOverlay, NULL, screen, &overlay1DstRect);
    renderThumb(screen);
  }
}

void incrementGameListFrame(const uint8_t frameAdvance)
{
  uint8_t frameCount = 0;
  /* Loop through logical frames until we match wall clock frames */
  for (frameCount = 0; frameCount < frameAdvance; frameCount++)
  {
    /* We accelerate the overlay's movement at first */
    if (currentIndex != nextIndex) {
      shiftFrame += shiftFrameFactor;

      /* Accelerate the overlay's movement at the start and end */
      if ((shiftFrame <= 6.0f) || (shiftFrame >= 28.0f))
        shiftFrameFactor += 0.1f;
    } /* End (currentIndex != nextIndex) */
  
    if (shiftFrame >= 38.0f /*(selectOverlay->h - 6)*/) {
      currentIndex = nextIndex;
      if (nextIndex < topIndex) topIndex = nextIndex;
      else if (nextIndex >= (topIndex + MAX_GAMES_PER_SCREEN)) topIndex++; 
      shiftFrame = 0.0f;
      shiftFrameFactor = BASE_SHIFT_FRAME_FACTOR;
      break; /* Break for loop */
    } /* End (shiftFrame >= 38.0f) */

  } /* End for loop */

}

void shiftSelectedGameDown(int step)
{
  /* Already shifting to a new index? */
  if (currentIndex != nextIndex)
    return;

  /* Are we at the end of the list? */
  if (currentIndex == (vGameInfo.size()-1))
    return;

  /* Start the shifting process */
  /* Case 1: Moving down one item */
  if (step == 1)
  {
    /* Adjust the textItem[] and render a new title */
    nextIndex = currentIndex + 1;
    renderTextItems(true, false); 
  }
  /* Case 2: Jumping a whole page of games */
  else
  {
    /* Are we already on the last page of games? */
    if (currentIndex >= (((vGameInfo.size() - 1) / MAX_GAMES_PER_SCREEN) *
      MAX_GAMES_PER_SCREEN) )
      return;  

    /* Jump to next "top of the screen" index */
    nextIndex = (currentIndex / MAX_GAMES_PER_SCREEN) *
      MAX_GAMES_PER_SCREEN;
    nextIndex += MAX_GAMES_PER_SCREEN;
    currentIndex = nextIndex;

    /* Last screen of games has less than MAX_GAMES_PER_SCREEN entries... */
    if ((currentIndex - MAX_GAMES_PER_SCREEN + 1) < (vGameInfo.size() - 1))
      topIndex = (vGameInfo.size() - MAX_GAMES_PER_SCREEN);
    else
      topIndex = currentIndex;

    /* Rerender the textItems to warp to the new location */
    renderTextItems(true, true);
  }
  playOverlaySnd();
}

void shiftSelectedGameUp(int step)
{
    /* Already shifting to a new index? */
  if (currentIndex != nextIndex)
    return;

  /* Are we at the start of the list? */
  if (currentIndex == 0)
    return;

  /* Start the shifting process */
  /* Case 1: Moving down one item */
  if (step == 1) {
    /* Do we need to adjust the textItem[] and render a new title? */
    nextIndex = currentIndex - 1;
    renderTextItems(false, false);
  }
  /* Case 2: Jumping a whole page of games */
  else
  {
    /* Are we already on the first page of games? */
    if (currentIndex < MAX_GAMES_PER_SCREEN)
      topIndex = currentIndex = nextIndex = 0;
    else {
      /* Jump to previous "top of the screen" index */
      nextIndex = (currentIndex / MAX_GAMES_PER_SCREEN) *
        MAX_GAMES_PER_SCREEN;
      nextIndex -= MAX_GAMES_PER_SCREEN;
      currentIndex = nextIndex;
      topIndex = currentIndex;
    }

    /* Rerender the textItems to warp to the new location */
    renderTextItems(false, true);
  }
  playOverlaySnd();
}

uint32_t currentSelectedGameIndex(void)
{
  return currentIndex;
}

uint32_t acceptButton(void)
{
  return (currentIndex == nextIndex);
}


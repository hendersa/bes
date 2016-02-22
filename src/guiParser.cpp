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


#include <stdio.h>
#include <string.h>
#include <expat.h>
#include "gui.h"

#define WORK_BUF_SIZE 256
char workingBuf[WORK_BUF_SIZE];
int workingIndex = 0;

unsigned char BESButtonMap[NUM_PLAYERS][BUTTON_MAP_SIZE] = {
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xff},
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xff} };

unsigned char GPIOButtonMap[GPIO_MAP_SIZE] = { 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

uint32_t BESPauseCombo = 0;

/* Head of the game information linked list */
gameInfo_t *gameInfo = NULL;

/* Temp nodes used in game information linked list */
gameInfo_t *prevGame = NULL;
gameInfo_t *currentGame = NULL;

typedef struct {
  const char *name; /* Name of the tag */
  const int depth;  /* Proper depth for tag use */
} TagInfo_t;

static TagInfo_t tagInfo[TAG_LAST] = { 
  { "config",  1 }, /* TAG_CONFIG */
  { "snes",    2 }, /* TAG_SNES */
  { "gba",     2 }, /* TAG_GBA */
  { "nes",     2 }, /* TAG_NES */
  { "gbc",     2 }, /* TAG_GBC */

  { "game",    3 }, /* TAG_GAME */
  { "title",   4 }, /* TAG_TITLE */
  { "rom",     4 }, /* TAG_ROM */
  { "image",   4 }, /* TAG_IMAGE */
  { "year",    4 }, /* TAG_YEAR */
  { "genre",   4 }, /* TAG_GENRE */
  { "text",    4 }, /* TAG_TEXT */

  { "player1", 2 }, /* TAG_CONTROL */
  { "player2", 2 }, /* TAG_PLAYER */
  { "vaxis",   3 }, /* TAG_VAXIS */
  { "haxis",   3 }, /* TAG_HAXIS */
  { "L",       3 }, /* TAG_LEFT */
  { "R",       3 }, /* TAG_RIGHT */
  { "A",       3 }, /* TAG_A */
  { "B",       3 }, /* TAG_B */
  { "X",       3 }, /* TAG_X */
  { "Y",       3 }, /* TAG_Y */
  { "select",  3 }, /* TAG_SELECT */
  { "start",   3 }, /* TAG_START */
  { "pause",   3 }, /* TAG_PAUSE (triggers pause menu) */

  { "gpio",         2 }, /* TAG_GPIO */
  { "gpio_gpleft",  3 }, /* TAG_GPIO_GPLEFT */
  { "gpio_gpright", 3 }, /* TAG_GPIO_GPRIGHT */
  { "gpio_gpup",    3 }, /* TAG_GPIO_GPUP */
  { "gpio_gpdown",  3 }, /* TAG_GPIO_GPDOWN */
  { "gpio_L",       3 }, /* TAG_GPIO_LEFT */
  { "gpio_R",       3 }, /* TAG_GPIO_RIGHT */
  { "gpio_A",       3 }, /* TAG_GPIO_A */
  { "gpio_B",       3 }, /* TAG_GPIO_B */
  { "gpio_X",       3 }, /* TAG_GPIO_X */
  { "gpio_Y",       3 }, /* TAG_GPIO_Y */
  { "gpio_select",  3 }, /* TAG_GPIO_SELECT */
  { "gpio_start",   3 }, /* TAG_GPIO_START */
  { "gpio_pause",   3 }, /* TAG_GPIO_PAUSE */ 

  { "pause_combo", 2}, /* TAG_PAUSE_COMBO */
  { "pause_key",   3}  /* TAG_PAUSE_KEY */
};

/* Platform that the current game is for */
platformType_t currentPlatform = PLATFORM_INVALID;
int currentPlayer = PLAYER_INVALID;

/* Flag for what tag we're currently in */
static int8_t inTagFlag[TAG_LAST];
/* Flags for the fields that have defined in this gameInfo node */
static int8_t definedTagFlag[TAG_LAST];

/* Counters for how many of a tag have been used in this "game" tag */
static uint8_t currentGenre = 0;
static uint8_t currentText = 0;

static void XMLCALL
characterData(void *userData, const char *data, int len)
{
  int i;

  /* Range check */
  if ((len + workingIndex + 1) > WORK_BUF_SIZE)
  {
    fprintf(stderr, "ERROR: Text is way too long in tag, truncating\n");
    len = WORK_BUF_SIZE - workingIndex - 1;
  }

  for (i=0; i < len; i++)
  {
    workingBuf[workingIndex] = data[i];
    workingIndex++;
  }
}

static void XMLCALL
startElement(void *userData, const char *name, const char **atts)
{
  int i;
  int *depthPtr = (int *)userData;

  /* Increase depth */
  *depthPtr += 1;

  memset(workingBuf, 0, WORK_BUF_SIZE);
  workingIndex = 0;

  for (i = TAG_FIRST; i < TAG_LAST; i++)
  {
    if (
      !strcasecmp(name, tagInfo[i].name) && /* Tag match? */ 
      (tagInfo[i].depth == *depthPtr)       /* Depth match? */ 
    )
    {
      /* Check to make sure this tag isn't open */
      if (!inTagFlag[i])
        /* We're in a tag, now */
        inTagFlag[i] = 1;
      else
      {
        fprintf(stderr, "ERROR: <%s> tag found when one already open\n", tagInfo[i].name);
      }
      /* Switch to the proper rule for the special tags */
      switch(i)
      {
        case TAG_GAME:
          /* Allocate a new info node */
          currentGame = new gameInfo_t(); 
          if (!currentGame) {
            fprintf(stderr, "\nERROR: Unable to allocate new game node\n");
            exit(1);
          }
          /* Defaults */
          currentGame->dateText.assign(DEFAULT_DATE_TEXT);
          currentGame->platform = currentPlatform;
          currentGenre = 0;
          currentText = 0;
          break;

        case TAG_GENRE:
          fprintf(stderr, "\n<%s>", tagInfo[i].name);
          break;

        case TAG_TEXT:
          fprintf(stderr, "\n<%s>", tagInfo[i].name);
          break;

        case TAG_SNES:
        case TAG_GBA:
        case TAG_NES:
        case TAG_GBC:
          fprintf(stderr, "\n<%s>", tagInfo[i].name);
          currentPlatform = (platformType_t)i;
          break;

        case TAG_PLAYER1:
        case TAG_PLAYER2:
          fprintf(stderr, "\n<%s>", tagInfo[i].name);
          currentPlayer = i - TAG_PLAYER1;
          break;

        default:
          fprintf(stderr, "\n<%s>", tagInfo[i].name);
          break;
      }
    }
  }
}

static void XMLCALL
endElement(void *userData, const char *name)
{
  int *depthPtr = (int *)userData;
  int i, x, invalidTag = 0;
  unsigned int control = 0;

  for (i = TAG_FIRST; i < TAG_LAST; i++)
  {
    if (
      !strcasecmp(name, tagInfo[i].name) && /* Tag match? */
      (tagInfo[i].depth == *depthPtr)       /* Depth match? */
    )
    {
      /* Is this the current, open tag? */
      if (inTagFlag[i])
      {
        fprintf(stderr, "</%s>", tagInfo[i].name);

        switch(i) {
          case TAG_CONFIG:
            break;
          case TAG_GAME:
            /* Are any tags still open? */
            for (x = TAG_GAME + 1; x < TAG_GAME_LAST; x++)
              if (inTagFlag[x]) invalidTag = 1; 
            if (invalidTag)
            {
              fprintf(stderr, "ERROR: Found </game> tag in incorrect place\n");
              if (currentGame)
              { 
                free(currentGame);
                currentGame = NULL;
              }
              break;
            }
            /* Do we have the bare minimum fields? */
            else if (definedTagFlag[TAG_TITLE] && definedTagFlag[TAG_ROM])
            {
              totalGames++;
              prevGame = gameInfo;
              while(prevGame->next)
              {
                if (prevGame->next->gameTitle != currentGame->gameTitle)
                //if (strcmp(prevGame->next->gameTitle, currentGame->gameTitle) > 0)
                  break;
                prevGame = prevGame->next;
              }
              currentGame->next = prevGame->next;
              prevGame->next = currentGame;
            }
            else 
            {
              fprintf(stderr, "ERROR: Missing game title or rom filename\n");
              if (currentGame)
              {
                free(currentGame);
                currentGame = NULL;
              }
            }

            /* Reset the flags for tag in use and defined */
            for (i = TAG_GAME_FIRST; i < TAG_GAME_LAST; i++) {
              inTagFlag[i] = 0;
              definedTagFlag[i] = 0;
            }
            break;

          case TAG_TITLE:
            if (definedTagFlag[i])
            {
              fprintf(stderr, "ERROR: Title already defined for game\n");
              break;
            }
            //fprintf(stderr, "Copying gameTitle '%s' into node\n", workingBuf);
            currentGame->gameTitle = workingBuf;
            //strncpy(currentGame->gameTitle, workingBuf, GAME_TITLE_SIZE - 1);
            definedTagFlag[i] = 1;
            break;

          case TAG_ROM:
            if (definedTagFlag[i])
            {
              fprintf(stderr, "ERROR: ROM already defined for game\n");
              break;
            }
            //fprintf(stderr, "Copying romFile '%s' into node\n", workingBuf);
            currentGame->romFile = workingBuf;
            //strncpy(currentGame->romFile, workingBuf, ROM_FILE_SIZE - 1);
            definedTagFlag[i] = 1;
            break;

          case TAG_IMAGE:
            if (definedTagFlag[i])
            {
              fprintf(stderr, "ERROR: Image already defined for game\n");
              break;
            }
            //fprintf(stderr, "Copying imageFile '%s' into node\n", workingBuf);
            currentGame->imageFile.assign(workingBuf);
            //strncpy(currentGame->imageFile, workingBuf, IMAGE_FILE_SIZE -1);
            definedTagFlag[i] = 1;
            break;

          case TAG_YEAR:
            if (definedTagFlag[i])
            {
              fprintf(stderr, "ERROR: Year already defined for game\n");
              break;
            }
            //fprintf(stderr, "Copying dateText '%s' into node\n", workingBuf);
            currentGame->dateText.assign(workingBuf);
            //strncpy(currentGame->dateText, workingBuf, DATE_TEXT_SIZE -1);
            definedTagFlag[i] = 1;
            break;

          case TAG_GENRE:
            if (currentGenre < MAX_GENRE_TYPES)
            {
              //fprintf(stderr, "Copying genreText[%d] '%s' into node\n", currentGenre, workingBuf);
              currentGame->genreText[currentGenre].assign(workingBuf);
              currentGenre++;
            }
            definedTagFlag[i] = 1;
            break;

          case TAG_TEXT:
            if (currentText < MAX_TEXT_LINES)
            {
              //fprintf(stderr, "Copying infoText[%d] '%s' into node\n", currentText, workingBuf);
              currentGame->infoText[currentText].assign(workingBuf);
              currentText++;
            }
            definedTagFlag[i] = 1;
            break;

          case TAG_SNES:
          case TAG_GBA:
          case TAG_NES:
          case TAG_GBC:
            definedTagFlag[i] = 0;
            currentPlatform = PLATFORM_INVALID;
            break;

          case TAG_PLAYER1:
          case TAG_PLAYER2:
            definedTagFlag[i] = 0;
            currentPlayer = PLAYER_INVALID;
            break;

          case TAG_VAXIS:
          case TAG_HAXIS:
          case TAG_LEFT:
          case TAG_RIGHT:
          case TAG_A:
          case TAG_B:
          case TAG_X:
          case TAG_Y:
          case TAG_SELECT:
          case TAG_START:
          case TAG_PAUSE:
            if ((currentPlayer == PLAYER_INVALID) || (currentPlayer >= NUM_PLAYERS))
            {
              fprintf(stderr, "ERROR: Found a </%s> tag outside of a <player?> tag\n", tagInfo[i].name);
            }
            else
            {
              sscanf(workingBuf, "%u", &control);
              BESButtonMap[currentPlayer][i - TAG_FIRST_CONTROL] = control; 
            }
            definedTagFlag[i] = 0;
            break;

          case TAG_PAUSE_COMBO:
            break;

          case TAG_PAUSE_KEY:
            sscanf(workingBuf, "%u", &control);
            BESPauseCombo |= (1 << control);
            fprintf(stderr, "Adding button %d to pause combo (%u)\n", control, BESPauseCombo);
            break;

          case TAG_GPIO:
            break;

          case TAG_GPIO_GPLEFT:
          case TAG_GPIO_GPRIGHT:
          case TAG_GPIO_GPUP:
          case TAG_GPIO_GPDOWN:
          case TAG_GPIO_LEFT:
          case TAG_GPIO_RIGHT:
          case TAG_GPIO_A:
          case TAG_GPIO_B:
          case TAG_GPIO_X:
          case TAG_GPIO_Y:
          case TAG_GPIO_SELECT:
          case TAG_GPIO_START:
          case TAG_GPIO_PAUSE:
            sscanf(workingBuf, "%u", &control);
            GPIOButtonMap[i - TAG_FIRST_GPIO_CONTROL] = control;
            fprintf(stderr, "GPIOButtonMap[%d] = %u\n", (i-TAG_FIRST_GPIO_CONTROL), control);
            break;

          default:
            fprintf(stderr, "ERROR: Close of undefined tag\n");
            break;
        } /* end switch */
        inTagFlag[i] = 0;
      }
      else
        fprintf(stderr, "Closing tag mismatch: <\\%s> found\n", tagInfo[i].name);
    }
  }
  /* Decrease depth */
  *depthPtr -= 1;
}

int loadGameConfig(void)
{
  char buf[BUFSIZ];
  XML_Parser parser = XML_ParserCreate(NULL);
  int done, i, depth = 0;
  FILE *config = NULL;

  gameInfo = NULL;
  totalGames = 0;

  XML_SetUserData(parser, &depth);
  XML_SetElementHandler(parser, startElement, endElement);
  XML_SetCharacterDataHandler(parser, characterData);

  config = fopen(BES_FILE_ROOT_DIR "/games.xml", "r");
  if (!config) {
    fprintf(stderr, "Unable to open game configuration file games.xml\n");
    return 1;
  }

  /* Initialize the flags for tag in use and defined */
  for (i = TAG_FIRST; i < TAG_LAST; i++) {
    inTagFlag[i] = 0;
    definedTagFlag[i] = 0;
  }

  /* Initialize the dummy head sentinel */
  //fprintf(stderr, "Creating sentinels\n");
  gameInfo = new gameInfo_t;
  if (!gameInfo) {
    fprintf(stderr, "\nERROR: Unable to allocate sentinel node\n");
    exit(1);
  } 
  currentGame = gameInfo;

  do {
    size_t len = fread(buf, 1, sizeof(buf), config);
    done = len < sizeof(buf);
    if (XML_Parse(parser, buf, len, done) == XML_STATUS_ERROR) {
      fprintf(stderr,
              "%s at line %lu\n",
              XML_ErrorString(XML_GetErrorCode(parser)),
              XML_GetCurrentLineNumber(parser));
      fclose(config);
      return 1;
    }
  } while (!done);
  XML_ParserFree(parser);
  fclose(config);
  return 0;
}


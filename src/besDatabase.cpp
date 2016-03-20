#include <unistd.h>
#include <stdio.h>
#include "sqlite3.h"
#include "gui.h"

#define DATABASE_NAME "games.db"

bool loadControlDatabase(void) {
  uint8_t i = 0;
  int deadzone = 0;
  int player = 0;
  sqlite3 *db = NULL;
  sqlite3_stmt *dbStatement = NULL;
  int dbCode = 0;
  std::string dbQuery;
  std::string pauseCombo;

  /* Open database */
  if (sqlite3_open(DATABASE_NAME, &db))
  {
    fprintf(stderr, "ERROR: Unable to open database '%s'\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    db = NULL;
    return false;
  }

  /* Prepare the database query */
  dbQuery = "SELECT * FROM Controls";
  if (sqlite3_prepare_v2(db, dbQuery.c_str(), dbQuery.length(), &dbStatement, NULL))
  {
    fprintf(stderr, "ERROR: Unable to prepare 'Controls' database query (%s)\n",
      sqlite3_errmsg(db));
    sqlite3_close(db);
    return false;
  }

  /* Run the query */
  dbCode = sqlite3_step(dbStatement);
  while (dbCode != SQLITE_DONE) {
    /* If the database is busy, wait 10us and try again */
    while (dbCode == SQLITE_BUSY)
    {
      usleep(10);
      dbCode = sqlite3_step(dbStatement);
    }

    /* Do we have a new row? */
    if (dbCode == SQLITE_ROW) {
      /* Player number for this record */
      player = (sqlite3_column_int(dbStatement, 15) - 1);
      /* L, R, A, B, X, Y, Select, Start, and Pause button numbers */
      for (i = 0; i < BUTTON_TOTAL; i++)
        BESButtonMap[player][i] = sqlite3_column_int(dbStatement, i);
      /* Vertical and horizontal axis numbers */
      for (i = 0; i < AXIS_TOTAL; i++)
        BESButtonMap[player][i+BUTTON_TOTAL] = 
          sqlite3_column_int(dbStatement, (BUTTON_TOTAL + i) );
      /* Axis invert settings */
      BESAxisMap[player][AXIS_VERTICAL][AXISSETTING_INVERT] = 
        sqlite3_column_int(dbStatement, 11);
      BESAxisMap[player][AXIS_HORIZONTAL][AXISSETTING_INVERT] =
        sqlite3_column_int(dbStatement, 12);
      /* deadzone setting in the db is 0 (20%), 1 (40%), or 2 (60%) */
      deadzone = sqlite3_column_int(dbStatement, 13) + 1;
      deadzone *= 6400;
      BESAxisMap[player][AXIS_VERTICAL][AXISSETTING_DEADZONE] = deadzone;
      BESAxisMap[player][AXIS_HORIZONTAL][AXISSETTING_DEADZONE] = deadzone;
      /* Pause combo (if there is one) */
      pauseCombo = std::string((const char *)sqlite3_column_text(dbStatement, 14));
      for (i = 0; i < pauseCombo.length(); i++)
        if (pauseCombo[i] == '1')
          BESPauseCombo |= (1 << i);

    /* Was there a problem with performing the query? */
    } else if (dbCode != SQLITE_DONE) {
      fprintf(stderr, "ERROR: Problem performing 'Controls' database query (%s), %d\n",
        sqlite3_errmsg(db), dbCode);
      sqlite3_close(db);
      return false;
    }
    dbCode = sqlite3_step(dbStatement);
  } /* End while */

  /* Done, so close the database */
  sqlite3_finalize(dbStatement);
  sqlite3_close(db);

fprintf(stderr, "Gamepad #1:\n");
  for(i=0; i < (BUTTON_TOTAL + AXIS_TOTAL); i++) 
    fprintf(stderr, "  [%d]: %d\n", i, BESButtonMap[0][i]);
fprintf(stderr, "\nGamepad #2:\n");
  for(i=0; i < (BUTTON_TOTAL + AXIS_TOTAL); i++)
    fprintf(stderr, "  [%d]: %d\n", i, BESButtonMap[1][i]);

  return true;
}

bool loadGPIODatabase(void) {
  return true;
}

#ifndef BUILD_SNES
bool loadGameDatabase(void) {
  uint8_t i = 0, x = 0;
  sqlite3 *db = NULL;
  sqlite3_stmt *dbStatement = NULL;
  int dbCode = 0;
  std::string dbQuery;
  gameInfo_t gameInfo;

  /* Clear out the gameInfo vector */
  vGameInfo.clear();

  /* Open database */
  if (sqlite3_open(DATABASE_NAME, &db))
  {
    fprintf(stderr, "ERROR: Unable to open database '%s'\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    db = NULL;
    return false;
  }

  /* Prepare the database query */
  dbQuery = "SELECT * FROM Games";
  if (sqlite3_prepare_v2(db, dbQuery.c_str(), dbQuery.length(), &dbStatement, NULL))
  {
    fprintf(stderr, "ERROR: Unable to prepare 'Games' database query (%s)\n",
      sqlite3_errmsg(db));
    sqlite3_close(db);
    return false;
  }

  /* Run the query */
  dbCode = sqlite3_step(dbStatement);
  while (dbCode != SQLITE_DONE) {
    /* If the database is busy, wait 10us and try again */
    while (dbCode == SQLITE_BUSY)
    {
      usleep(10);
      dbCode = sqlite3_step(dbStatement);
    }

    /* Do we have a new row? */
    if (dbCode == SQLITE_ROW) {
      /* Populate a gameInfo_t struct with the row of data */
      x = 0;
      gameInfo.gameTitle = std::string((const char *)sqlite3_column_text(dbStatement, x++));
      gameInfo.romFile = std::string((const char *)sqlite3_column_text(dbStatement, x++));
      for (i=0; i < MAX_TEXT_LINES; i++)
        gameInfo.infoText[i] = std::string((const char *)sqlite3_column_text(dbStatement, x++)); 
      gameInfo.dateText = std::string((const char *)sqlite3_column_text(dbStatement, x++));
      for (i=0; i < MAX_GENRE_TYPES; i++)
        gameInfo.genreText[i] = std::string((const char *)sqlite3_column_text(dbStatement, x++));
      gameInfo.platform = (enumPlatform_t)(sqlite3_column_int(dbStatement, x++) - 1);
      
      /* Store the struct in the vector */
      vGameInfo.push_back(gameInfo);

    /* Was there a problem with performing the query? */
    } else if (dbCode != SQLITE_DONE) {
      fprintf(stderr, "ERROR: Problem performing game database query (%s), %d\n",
        sqlite3_errmsg(db), dbCode);
      sqlite3_close(db);
      return false;
    }
    dbCode = sqlite3_step(dbStatement);
  } /* End while */

  /* Done, so close the database */
  sqlite3_finalize(dbStatement);
  sqlite3_close(db);
  return true;
}
#endif /* BUILD_SNES */


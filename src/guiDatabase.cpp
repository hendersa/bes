#include <unistd.h>
#include <stdio.h>
#include "sqlite/sqlite3.h"
#include "gui.h"

#define DATABASE_NAME "games.db"

bool loadGameDatabase(void) {
  uint8_t i = 0, x = 0;
  sqlite3 *db = NULL;
  sqlite3_stmt *dbStatement = NULL;
  int dbCode = 0;
  std::string dbQuery;
  gameInfo_t gameInfo;

  /* Clear out the gameInfo vector */
  vGameInfo.clear();

  /* Open games database */
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
    fprintf(stderr, "ERROR: Unable to prepare game database query (%s)\n",
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
      gameInfo.platform = (platformType_t)sqlite3_column_int(dbStatement, x++);
      
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


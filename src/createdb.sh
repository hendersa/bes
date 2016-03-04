#!/bin/sh
rm -f ../games.db

# Create an empty games.db
sqlite3 ../games.db 'CREATE TABLE Games ( GameTitle varchar NOT NULL, RomFile varchar NOT NULL, InfoText0 varchar NOT NULL, InfoText1 varchar NOT NULL, InfoText2 varchar NOT NULL, InfoText3 varchar NOT NULL, InfoText4 varchar NOT NULL, DateText varchar NOT NULL, Genre0 varchar NOT NULL, Genre1 varchar NOT NULL, Platform tinyint NOT NULL )'


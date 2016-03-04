#!/bin/sh
./createdb.sh

# One full entry for each platform
sqlite3 ../games.db 'INSERT INTO Games VALUES ("SNES Title", "dummy.sfc", "Line1", "Line2", "Line3", "Line4", "Line5", "1999", "Genre1", "Genre2", 1 )'
sqlite3 ../games.db 'INSERT INTO Games VALUES ("GBA TITLE", "dummy.gb", "Line1", "Line2", "Line3", "Line4", "Line5", "1999", "Genre1", "Genre2", 2 )'
sqlite3 ../games.db 'INSERT INTO Games VALUES ("NES TITLE", "dummy.nes", "Line1", "Line2", "Line3", "Line4", "Line5", "1999", "Genre1", "Genre2", 3 )'
sqlite3 ../games.db 'INSERT INTO Games VALUES ("GBC TITLE", "dummy.gba", "Line1", "Line2", "Line3", "Line4", "Line5", "1999", "Genre1", "Genre2", 4 )'

# Test entries for default values
sqlite3 ../games.db 'INSERT INTO Games VALUES ("Default Text Entry", "dummy.gba", "", "", "", "", "", "1999", "Genre1", "Genre2", 4 )'
sqlite3 ../games.db 'INSERT INTO Games VALUES ("Default Date Entry", "dummy.gba", "Line1", "Line2", "Line3", "Line4", "Line5", "", "Genre1", "Genre2", 4 )'
sqlite3 ../games.db 'INSERT INTO Games VALUES ("One Genre", "dummy.gba", "Line1", "Line2", "Line3", "Line4", "Line5", "1999", "Genre1", "", 4 )'
sqlite3 ../games.db 'INSERT INTO Games VALUES ("No Genre", "dummy.gba", "Line1", "Line2", "Line3", "Line4", "Line5", "1999", "", "", 4 )'

sqlite3 ../games.db 'SELECT * FROM Games'


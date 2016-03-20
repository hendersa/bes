#!/bin/sh
./createdb.sh

# GPIO controller settings
sqlite3 ../games.db 'INSERT INTO GPIO VALUES ( 45, 44, 23, 26, 47, 46, 65, 27, 22, 66, 67, 69, 68 )'
 
# Test controller settings
sqlite3 ../games.db 'INSERT INTO Controls VALUES ( 4, 5, 1, 2, 0, 3, 9, 8, -1, 0, 1, 0, 0, 0, "11000011", 1 )'

sqlite3 ../games.db 'INSERT INTO Controls VALUES ( 4, 5, 1, 2, 0, 3, 9, 8, -1, 0, 1, 0, 0, 0, "11000011", 2 )'

# One full entry for each platform
sqlite3 ../games.db 'INSERT INTO Games VALUES ("SNES Title", "dummy.sfc", "Line1", "Line2", "Line3", "Line4", "Line5", "1999", "Action", "Fantasy", 1 )'
sqlite3 ../games.db 'INSERT INTO Games VALUES ("GBA TITLE", "dummy.gba", "Line1", "Line2", "Line3", "Line4", "Line5", "1999", "Action", "Fantasy", 2 )'
sqlite3 ../games.db 'INSERT INTO Games VALUES ("NES TITLE", "dummy.nes", "Line1", "Line2", "Line3", "Line4", "Line5", "1999", "Action", "Fantasy", 3 )'
sqlite3 ../games.db 'INSERT INTO Games VALUES ("GBC TITLE", "dummy.gb", "Line1", "Line2", "Line3", "Line4", "Line5", "1999", "Action", "Fantasy", 4 )'

# Test entries for default values
sqlite3 ../games.db 'INSERT INTO Games VALUES ("Default Text Entry", "dummy1.gb", "", "", "", "", "", "1999", "RPG", "Sci-Fi", 4 )'
sqlite3 ../games.db 'INSERT INTO Games VALUES ("Default Date Entry", "dummy2.gb", "Line1", "Line2", "Line3", "Line4", "Line5", "", "Simulation", "Run and Gun", 4 )'
sqlite3 ../games.db 'INSERT INTO Games VALUES ("One Genre", "dummy3.gb", "Line1", "Line2", "Line3", "Line4", "Line5", "1999", "Action", "", 4 )'
sqlite3 ../games.db 'INSERT INTO Games VALUES ("No Genre", "dummy4.gb", "Line1", "Line2", "Line3", "Line4", "Line5", "1999", "", "", 4 )'

# Real entry
sqlite3 ../games.db 'SELECT * FROM Games'
sqlite3 ../games.db 'SELECT * FROM Controls'
sqlite3 ../games.db 'SELECT * FROM GPIO'


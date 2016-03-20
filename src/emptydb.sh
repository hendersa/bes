#!/bin/sh
./createdb.sh

# Default GPIO controller settings
sqlite3 ../games.db 'INSERT INTO GPIO VALUES ( 45, 44, 23, 26, 47, 46, 65, 27, 22, 66, 67, 69, 68 )'
 
# Default controller settings
sqlite3 ../games.db 'INSERT INTO Controls VALUES ( 4, 5, 1, 2, 0, 3, 9, 8, -1, 0, 1, 0, 0, 0, "11000011", 1 )'

sqlite3 ../games.db 'INSERT INTO Controls VALUES ( 4, 5, 1, 2, 0, 3, 9, 8, -1, 0, 1, 0, 0, 0, "11000011", 2 )'

# Dump out all data in database
sqlite3 ../games.db 'SELECT * FROM Games'
sqlite3 ../games.db 'SELECT * FROM Controls'
sqlite3 ../games.db 'SELECT * FROM GPIO'


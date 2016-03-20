#!/bin/sh
rm -f ../games.db

# Create an empty games.db
sqlite3 ../games.db 'CREATE TABLE Games ( GameTitle varchar NOT NULL, RomFile varchar NOT NULL, InfoText0 varchar NOT NULL, InfoText1 varchar NOT NULL, InfoText2 varchar NOT NULL, InfoText3 varchar NOT NULL, InfoText4 varchar NOT NULL, DateText varchar NOT NULL, Genre0 varchar NOT NULL, Genre1 varchar NOT NULL, Platform tinyint NOT NULL, PRIMARY KEY (RomFile, Platform) )'

sqlite3 ../games.db 'CREATE TABLE Controls ( LButton tinyint NOT NULL, RButton tinyint NOT NULL, AButton tinyint NOT NULL, BButton tinyint NOT NULL, XButton tinyint NOT NULL, YButton tinyint NOT NULL, StartButton tinyint NOT NULL, SelectButton tinyint NOT NULL, PauseButton tinyint NOT NULL, XAxis tinyint NOT NULL, YAxis tinyint NOT NULL, InvertX tinyint NOT NULL, InvertY tinyint NOT NULL, AnalogCutoff tinyint NOT NULL, PauseCombo varchar NOT NULL, Gamepad tinyint NOT NULL, PRIMARY KEY (Gamepad) )'

sqlite3 ../games.db 'CREATE TABLE GPIO ( LButton tinyint NOT NULL, RButton tinyint NOT NULL, AButton tinyint NOT NULL, BButton tinyint NOT NULL, XButton tinyint NOT NULL, YButton tinyint NOT NULL, StartButton tinyint NOT NULL, SelectButton tinyint NOT NULL, PauseButton tinyint NOT NULL, LDPad tinyint NOT NULL, RDPad tinyint NOT NULL, UDPad tinyint NOT NULL, DDPad tinyint NOT NULL )'
 

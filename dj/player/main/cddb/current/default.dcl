# default.dcl: default configuration for cddb test program on dharma
# edwardm@fullplaymedia.com 4/04/02
# (c) Interactive Objects

name cddb
type main

requires cddb_extras debug_util eventq_util events_core keyboard_dev 

export CDDBHelper.h CDDBEvents.h

compile CDDBHelper.cpp
compile branding.c clookup.c cmdhandlers.c command_table.c configuration.c
compile data_translator.c display.c logging.c lookup.c olookup.c pc_conn.c puffy_g.c
compile shell.c updates.c xlookup.c
#compile clookup.c data_translator.c 

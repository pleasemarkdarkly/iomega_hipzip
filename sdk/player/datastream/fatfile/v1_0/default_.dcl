# default.dcl: default configuration for fat file interface
# danc@iobjects.com 07/09/01
# (c) Interactive Objects

# this version compiles in both input and output support. you could
# easily make a config file that only built one or the other

name fatfile
type datastream

# general requirements
requires fat_fs

# input stream specific
requires input_datastream

# output stream specific
requires output_datastream

export FatFile.h FileInputStream.h FileOutputStream.h

link default.a

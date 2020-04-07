# default.dcl: default configuration for the cddb abstract layer module
# edwardm@iobjects.com 4/04/2002

name cddb_abstract_layer_dharma
type extras

requires cddb_extras fat_fs

compile gn_crypt.c gn_device_id.c gn_fs.c gn_memory.c


# Uncomment the following to build the struture validation test
#tests validate_alignment.c

# Uncomment the following to build the memory test
#export test_mem_struct.h
#compile lookup_table.c
#compile update_table.c
#tests test_mem.c

# Uncomment the following to build the file system test
#export test_fs.h
#compile check_api.c fake_update.c
#tests test_fs.c

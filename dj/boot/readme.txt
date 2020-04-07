copy dj.img from build directory

to build the 'GZIP Run loader', run 
./fisutil djconfig.fis 

This will generate a 2mb bin, dj image that'll work with the updater
------------------

if a flash fails, it will run a backup image (that'll only program the main image)

running the backup can be forced by holding down cancel during boot
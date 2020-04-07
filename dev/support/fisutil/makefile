#*****************************************************************************
#
# MAKEFILE - Builds the image creation utility
#
#*****************************************************************************

#
# The default rule, which builds the download utility.
#
all: fisutil.exe

#
# A rule to build the download utility.
#
fisutil.exe: fisutil.c fis.h
	gcc -O2 fisutil.c -o fisutil.exe


#
# A rule to remove the build products.
#
clean:
	@rm -f fisutil.exe
	@rm -f *.obj
	@rm -f *.o
	@rm -f *~

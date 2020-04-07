# default.dcl: default configuration for peg on dharma
# ericg@iobjects.com 8/31/01

name peg
type gui

requires screendriver_peg 

compile pbitmaps.cpp pdecwin.cpp pfonts.cpp picon.cpp pliteral.cpp
compile ppresent.cpp prect.cpp pscreen.cpp pstring.cpp ptextbox.cpp 
compile pthing.cpp pwindow.cpp pmessage.cpp

export peg.hpp pconfig.hpp pdecwin.hpp pegkeys.hpp pegtypes.hpp
export pfonts.hpp picon.hpp pmessage.hpp ppresent.hpp pscreen.hpp
export pstring.hpp ptextbox.hpp pthing.hpp pwindow.hpp

tests peg_tests.cpp

arch pbitmaps.o pdecwin.o pfonts.o picon.o pliteral.o pmessage.o
arch ppresent.o prect.o pscreen.o pstring.o ptextbox.o pthing.o pwindow.o


dist include/peg.hpp include/pconfig.hpp include/pdecwin.hpp
dist include/pfonts.hpp include/picon.hpp include/pmessage.hpp
dist include/pstring.hpp include/ptextbox.hpp include/pthing.hpp
dist include/pegkeys.hpp include/pegtypes.hpp include/ppresent.hpp
dist include/pwindow.hpp include/pscreen.hpp
dist default.a

header _screendriver.h start
#define PSCREEN  <gui/peg/screendriver/monoscrn.hpp>
header _screendriver.h end

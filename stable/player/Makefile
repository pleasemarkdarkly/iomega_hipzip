#
# <product>/player/Makefile
# <product>/builds/<target>/Makefile: player build makefile
# danc@iobjects.com 6/13/01
# (c) Interactive Objects
#

#
# There are a few basic ways to use this makefile:
# 1) implicit target (or 'make all'):
#    create an .exe from all the objects in the build
# 2) test target (as in 'make TEST=<filename>'):
#    build an .exe from the objects in the build and the given test target
# 3) tests (as in 'make fat_tests'):
#    build an .exe for each of the test targets for the given module
# 4) archives (as in 'make ARCHIVE=<archive target>'):
#    build a .a for the given archive target
#


#
# determine which directory we are in; if we are in the build
# directory, execute normally. otherwise error out
# implicit is that no source directory will have the "build" keyword in it,
# and every build directory will have the "build" keyword in it
#

.SUFFIXES:

ifeq (,$(filter %builds/,$(dir $(CURDIR))))

#
# optionally this section of code could attempt to determine where the
# build tree is, and invoke itself there. doing that would probably involve
# making the rest of the makefile conditional and putting a catch all rule here
#

$(error Please invoke this makefile from your build directory $(FILT))

endif

#
# Makefile begins
#

.PHONY: all dist make_image make_archive clean depclean testclean fullclean config pre_build

#
# the first rule encountered is 'all', which either invokes
#  'make_image', 'make_test', or 'make_archive'
#

ifdef ARCHIVE
BUILD_TARG := make_archive
else
BUILD_TARG := make_image
endif


all: pre_build $(BUILD_TARG)

#
# include our generated config information
#

COMPILER_FLAGS :=
LINKER_FLAGS :=
BUILD_TARGET :=

include _config.mk

#
# values that are set by the individual module.mk files
#

SRC    :=
DIST   :=
LIBS   :=
EXTENDED_LIBS :=
LINK_FLAGS := -Ttarget.ld -nostdlib -nostartfiles

#
# include our modules
#

ifdef TEST

# if we're building a test, filter out the main module,
# which defines cyg_user_start

MODS := $(patsubst $(MAIN_MODULE),,$(MODULES))

else

MODS := $(MODULES)

endif

include $(patsubst %, %/_module.mk, $(MODS))

#
# generate a list of objs/deps to build
#

OBJS := $(patsubst %.cxx,%.o,$(filter %.cxx,$(SRC)))
OBJS += $(patsubst %.cpp,%.o,$(filter %.cpp,$(SRC)))
OBJS += $(patsubst %.cc,%.o,$(filter %.cc,$(SRC)))
OBJS += $(patsubst %.c,%.o,$(filter %.c,$(SRC)))
OBJS += $(patsubst %.S,%.o,$(filter %.S,$(SRC)))
OBJS += $(patsubst %.s,%.o,$(filter %.s,$(SRC)))

DEPS := $(OBJS:.o=.d)

#
# generate a flat list of available tests
#

TESTOBJS := $(patsubst %.cxx,%.o,$(filter %.cxx,$(tests)))
TESTOBJS += $(patsubst %.cpp,%.o,$(filter %.cpp,$(tests)))
TESTOBJS += $(patsubst %.cc,%.o,$(filter %.cc,$(tests)))
TESTOBJS += $(patsubst %.c,%.o,$(filter %.c,$(tests)))
TESTOBJS += $(patsubst %.S,%.o,$(filter %.S,$(tests)))
TESTOBJS += $(patsubst %.s,%.o,$(filter %.s,$(tests)))

TESTDEPS := $(TESTOBJS:.o=.d)

#
# variables needed for the build process
#

# establish the VPATH as pointing to the source tree
VPATH = $(SRC_TREE)

# bash apparently doesn't set this by default ?
SHELL := /bin/bash

BUILD_LIST := _build_list
DIST_LIST  := _dist_list

PREFIX = arm-elf-
BASE_FLAGS = -mcpu=arm7tdmi -D__ECOS -D__EDB7209 -D__EDB7212 -D__EDB7312 -I$(ECOS_TREE)/include -I.

ifdef COMPILER_FLAGS
CFLAGS = $(BASE_FLAGS) $(COMPILER_FLAGS)
else
CFLAGS = $(BASE_FLAGS) -g -O2
endif

XCC = $(PREFIX)gcc
XCXX = $(PREFIX)g++
LD = $(XCXX)
AR = $(PREFIX)ar
STRIP = $(PREFIX)strip

CXXFLAGS = $(CFLAGS)
EXTRACFLAGS = -Wall -ffunction-sections -fdata-sections
EXTRACXXFLAGS = $(EXTRACFLAGS) -fno-exceptions -fno-rtti -finit-priority

LDFLAGS = -L$(ECOS_TREE)/lib $(LINK_FLAGS) $(LINKER_FLAGS) -Wl,--gc-sections

ARFLAGS = rcs
STRIPFLAGS = --strip-debug

#
# the files in our ecos/lib directory that we need deps on
#
ECOS_FILES_RAW = extras.o libextras.a libtarget.a target.ld vectors.o
ECOS_FILES = $(patsubst %,$(ECOS_TREE)/lib/%, $(ECOS_FILES_RAW))


#
# pull a little make magic to determine the version of gcc we are running.
# on a pre 3.0 setup, do a little extra work to fix some dep creation bugs
#

GCC_VERSION := $(shell $(XCC) --version)
GCC_2 := $(patsubst 2.%,2,$(filter 2.%,$(GCC_VERSION)))
GCC_3 := $(patsubst 3.%,3,$(filter 3.%,$(GCC_VERSION)))

ifeq (2, $(GCC_2))
GCC_MAJOR := 2
else
GCC_MAJOR := 3
endif

ifeq (3, $(GCC_MAJOR))
CXXFLAGS += -ffreestanding -fmessage-length=0
LDFLAGS += -lstdc++
else
CXXFLAGS += -fkeep-inline-functions
endif


#
# Macros
#

#
# macros for cleaning and configuring the tree
#

define clean-tests
	rm -f $(TESTOBJS) $(TESTOBJS:.o=.exe)
endef

define clean-testdeps
	rm -f $(TESTDEPS)
endef

define clean-objs
	rm -f $(OBJS) $(TARGET_FILE_NAME)
endef

define clean-deps
	rm -f $(DEPS)
endef

define clean-tree
	$(clean-objs); $(clean-deps); $(clean-tests); $(clean-testdeps)
endef

# dc 11/19/01  increment BUILD_VERSION. this isn't perfect, since it only increments on
#              config instead of build
define config-tree
	cd $(BASE_DIR) && ./scripts/parse_dcl.pl $(TARGET_NAME) $$(($(BUILD_VERSION) + 1))
endef

# dc 12/10/01  define the link-step as a macro, based on whether or not we link in
#              libraries. if we dont link libraries, we can use a faster link

ifeq "a$(EXTENDED_LIBS)" "a"
# no extended libs, use faster link
define link-objects
	$(XCXX) -nostdlib -o $(BUILD_TARGET) $(patsubst $(ECOS_FILES),,$^) $(LDFLAGS)
endef

else
# libs present, use slow link
define link-objects
	$(XCXX) -nostdlib -Wl,-r -Wl,--whole-archive -o $(BUILD_TARGET:.exe=.tmp) $(patsubst $(ECOS_FILES),,$^)
	$(XCXX) -o $(BUILD_TARGET) $(BUILD_TARGET:.exe=.tmp) $(LDFLAGS)
endef

endif

#
# you can optionally implement 'pre_build_step', 'post_build_step', 'post_link_step',
# 'pre_compile_step' and 'post_compile_step' as macros.
# if you choose not to, you get the default ones
#

ifndef pre_build_step
define pre_build_step
endef
endif

ifndef post_build_step
define post_build_step
endef
endif

ifndef post_link_step
define post_link_step
endef
endif

ifndef pre_compile_step
define pre_compile_step
endef
endif

ifndef post_compile_step
define post_compile_step
endef
endif


#
# Rules
#

#
# Stub rules and config rules
#

# dc 08/21/01  add stub rules for lib and ld files (eCos deps)
# dc 09/18/01  add stub rule for dcl files
%.ld :: ;
%.a :: ;
%.dcl :: ;
%.mk :: ;

# dc 07/19/01  dont reconfig if config is the cmd goal
#              this fixes a bug where config has been updated and 'make config'
#              is invoked, which formerly caused the system to reconfig twice
# dc 07/26/01  dont check out _config.mk dependencies if we are recursing
# dc 09/07/01  if recurse is defined, then MAKECMDGOALS isn't, so switch order
# dc 10/22/01  use MAKELEVEL to determine if we are recursing
# dc 02/06/02  if the _config.mk is updated, assume the build flags changed and
#              clean the tree
# dc 04/30/02  use a dependency list for module files, rather than a single file

ifeq ($(MAKELEVEL),0)
ifneq ($(MAKECMDGOALS),config)

_config.mk: $(CONFIG_LIST_FILE) $(MODULE_DEP_LIST) $(CONFIG_FILES)
	@echo "Synching tree..."
	@if [ "$(CONFIG_LIST_FILE)" -nt "_config.mk" ]; then $(clean-objs); fi
	@if [ "$(CONFIG_LIST_FILE)" -nt "_config.mk" ]; then $(clean-deps); fi
	@if [ "$(CONFIG_LIST_FILE)" -nt "_config.mk" ]; then $(clean-tests); fi
	@if [ "$(CONFIG_LIST_FILE)" -nt "_config.mk" ]; then $(clean-testdeps); fi
	$(config-tree)

endif
endif


#
# Rules for cleaning the tree
#

clean:
	$(clean-objs)

depclean:
	$(clean-objs)
	$(clean-deps)

testclean:
	$(clean-tests)

fullclean:
	$(clean-objs)
	$(clean-deps)
	$(clean-tests)
	$(clean-testdeps)

config:
	@if [ "$(CONFIG_LIST_FILE)" -nt "_config.mk" ]; then $(clean-objs); fi
	@if [ "$(CONFIG_LIST_FILE)" -nt "_config.mk" ]; then $(clean-deps); fi
	@if [ "$(CONFIG_LIST_FILE)" -nt "_config.mk" ]; then $(clean-tests); fi
	@if [ "$(CONFIG_LIST_FILE)" -nt "_config.mk" ]; then $(clean-testdeps); fi
	$(config-tree)


#
# pre_build: execute the pre_build_step
#  note that this is always executed, even if nothing is built
#
pre_build:
	$(pre_build_step)

#
# make_archive: generate a library from a set of objects
#  note that the pre_build_step is currently executed, but the post build and
#  link steps aren't executed when making an archive
#

ARCHIVE_FILE_NAME := $($(ARCHIVE)_name).a

make_archive: $(ARCHIVE_FILE_NAME)

$(ARCHIVE_FILE_NAME): $($(ARCHIVE))
	$(STRIP) $(STRIPFLAGS) $($(ARCHIVE))
	$(AR) $(ARFLAGS) $($(ARCHIVE)_name).a $($(ARCHIVE))
	@[ $$? -eq 0 ] && echo $($(ARCHIVE)_name).a >> $(BUILD_LIST)


#
# make_image: generate an image
# note that make_image is a phony, so it ends up just listing the
# real target as a dep (otherwise a relink always occurs)
#

BUILD_TARGET := $(TARGET_FILE_NAME)

# if we are building a test, do some setup work to build the test target
ifdef TEST

TEST_TARGET := $(patsubst %.c,%.o,$(filter %.c,$(TEST)))
TEST_TARGET += $(patsubst %.cpp,%.o,$(filter %.cpp,$(TEST)))
TEST_TARGET += $(patsubst %.S,%.o,$(filter %.S,$(TEST)))

BUILD_TARGET = $(TEST_TARGET:.o=.exe)

OBJS += $(TEST_TARGET)

# 10/15/01 dc  include the test target dependency file
-include $(TEST_TARGET:.o=.d)

endif   # ifdef TEST


make_image: $(BUILD_TARGET)

# 11/30/01 dc  force a relink if ecos has been rebuilt
$(BUILD_TARGET): $(ECOS_FILES) $(OBJS) $(LIBS) $(EXTENDED_LIBS)
	$(post_build_step)
	$(link-objects)
	@rm -f $(BUILD_TARGET:.exe=.tmp)
	@[ $$? -eq 0 ] || exit 1
	@echo $(BUILD_TARGET) > $(BUILD_LIST)
	$(post_link_step)


#
# %_tests: rebuild the given set of tests
#

tests %_tests:
	@echo "Recursively building all tests for $@"
	@if [ "a$($@)" = "a" ]; then echo "No tests found"; exit 1; fi
	@echo "Test list: $($@)"
	@rm -f $(BUILD_LIST)
	@for i in $($@); do        \
	  make TEST=$$i;           \
	  [ $$? -eq 0 ] || exit 1; \
	done

#
# %_archive, %_archives: rebuild the given set of archives
#  unfortunately, %_archive has to recurse since $($@) can't be listed as a dep :(

%_archive:
	@if [ "a$($@_name)" = "a" ]; then echo "No archive name found"; exit 1; fi
	@rm -f $(BUILD_LIST)
	@make ARCHIVE=$@

archives %_archives:
	@echo "Building $@"
	@if [ "a$($@)" = "a" ]; then echo "No archives found"; exit 1; fi
	@rm -f $(BUILD_LIST)
	@for i in $($@); do	      \
	  make ARCHIVE=$$i;           \
	  [ $$? -eq 0 ] || exit 1;    \
	done


#
# dist: generate the distribution list
#  this step does not ensure the targets are built, but will verify where they are
#  (either in the player or build tree) and generate a list with relative paths

dist:
	@echo "Generating distribution list"
	@echo -n "" > $(DIST_LIST)
	@for i in $(DIST); do                            \
	  if [ ! -f $$i ]; then                          \
	    if [ ! -f "$(SRC_TREE)/$$i" ]; then          \
	      echo "Could not find file $$i";            \
            fi ;                                         \
          fi ;                                           \
	  DIST_FILE=$$i;                                 \
	  echo $$DIST_FILE >> $(DIST_LIST) ;             \
         done

#
# macro to generate deps
# gcc 2.xx doesn't prefix the filename with the path in the output dep file,
#  so we define make-dep to do this for us
# gcc 3.xx had a bug logged against it to fix it, but apparently the fools
# didn't do it :(
#

#ifeq (2,$(GCC_MAJOR))

define make-dep
	@if [ $$? -eq 0 ]; then \
	$(SHELL) -ec ' \
	echo -n $(dir $*) > $*.P; \
	cat $*.d >> $*.P; \
	  sed -e "s/#.*//" -e "s/^[^:]*: *//" -e "s/ *\\$$//" \
	      -e "/^$$/ d" -e "s/$$/ :/" < $*.d >> $*.P; \
	mv $*.P $*.d'; \
	fi
	@[ $$? -eq 0 ] || rm -f $*.d
endef

#else  # 2 == GCC_MAJOR
#
#define make-dep
#	@if [ $$? -eq 0 ]; then \
#	$(SHELL) -ec ' \
#	cp $*.d $*.P; \
#	  sed -e "s/#.*//" -e "s/^[^:]*: *//" -e "s/ *\\$$//" \
#	      -e "/^$$/ d" -e "s/$$/ :/" < $*.d >> $*.P; \
#	mv $*.P $*.d'; \
#	fi
#	@[ $$? -eq 0 ] || rm -f $*.d
#endef

#endif # 2 == GCC_MAJOR

#
# include the deps
#

ifneq ($(MAKECMDGOALS),depclean)
ifneq ($(MAKECMDGOALS),config)
-include $(DEPS)
endif
endif

#
# rules that dictate how to build source and dep files
# note that -MD causes gcc to compile and generate deps in the same step
# these rules are generally ugly because of a couple bugs in gcc dep generation
#

%.o: %.S
	$(pre_compile_step)
	$(XCC) -Wp,-MD,$*.d -c -o $*.o $(CFLAGS) $(EXTRACFLAGS) $< || (rm -f $*.o && exit 1)
	$(post_compile_step)
	$(make-dep)
%.o: %.s
	$(pre_compile_step)
	$(XCC) -Wp,-MD,$*.d -c -o $*.o $(CFLAGS) $(EXTRACFLAGS) $< || (rm -f $*.o && exit 1)
	$(post_compile_step)
	$(make-dep)
%.o: %.c
	$(pre_compile_step)
	$(XCC) -Wp,-MD,$*.d -c -o $*.o $(CFLAGS) $(EXTRACFLAGS) $< || (rm -f $*.o && exit 1)
	$(post_compile_step)
	$(make-dep)
%.o: %.cc
	$(pre_compile_step)
	$(XCXX) -Wp,-MD,$*.d -c -o $*.o $(CXXFLAGS) $(EXTRACXXFLAGS) $< || (rm -f $*.o && exit 1)
	$(post_compile_step)
	$(make-dep)
%.o: %.cxx
	$(pre_compile_step)
	$(XCXX) -Wp,-MD,$*.d -c -o $*.o $(CXXFLAGS) $(EXTRACXXFLAGS) $< || (rm -f $*.o && exit 1)
	$(post_compile_step)
	$(make-dep)
%.o: %.cpp
	$(pre_compile_step)
	$(XCXX) -Wp,-MD,$*.d -c -o $*.o $(CXXFLAGS) $(EXTRACXXFLAGS) $< || (rm -f $*.o && exit 1)
	$(post_compile_step)
	$(make-dep)

#
# rules that dictate how to generate preprocessed files; these files
# have been run through the preprocessor but have not been compiled
#

%.i: %.S
	$(XCC) -E -o $*.i $(CFLAGS) $(EXTRACFLAGS) $<
%.i: %.s
	$(XCC) -E -o $*.i $(CFLAGS) $(EXTRACFLAGS) $<
%.i: %.c
	$(XCC) -E -o $*.i $(CFLAGS) $(EXTRACFLAGS) $<
%.i: %.cc
	$(XCXX) -E -o $*.i $(CXXFLAGS) $(EXTRACXXFLAGS) $<
%.i: %.cxx
	$(XCXX) -E -o $*.i $(CXXFLAGS) $(EXTRACXXFLAGS) $<
%.i: %.cpp
	$(XCXX) -E -o $*.i $(CXXFLAGS) $(EXTRACXXFLAGS) $<



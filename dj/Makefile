#
# <product>/Makefile
# generic makefile to force kernel configuration and compilation,
# player configuration and compilation
#
# danc@iobjects.com 6/08/01
# (c) Interactive Objects
#

#
# this makefile runs off a list of possible targets provided by the
# targets.mk file in the current directory. targets.mk is customized
# for each product. users can have local targets by 
#

include configs/targets.mk
-include configs/custom_targets.mk

#
# regardless of the argument, the all: target is always invoked
# first, run ecos/Makefile with the argument we were passed to invoke
# the kernel generation process. then, invoke ./Makefile.player to
# configure and build the player.
#

.SUFFIXES:

TARGS := $(TARGETS) $(CUSTOM_TARGETS)
CLEAN_TARGS := $(patsubst %,%_clean, $(TARGETS))
CLEAN_TARGS += $(patsubst %,%_clean, $(CUSTOM_TARGETS))

MAKE_ARGS := -f Makefile.ecos -j"$$NUMBER_OF_PROCESSORS" -I configs

Makefile :: ;
%.mk :: ;

$(CLEAN_TARGS) ::
	@echo "Building target $(@) clean"
	@$(MAKE) $(MAKE_ARGS) TARGET=$(patsubst %_clean,%,$(@)) clean
	@$(MAKE) -r -C builds/$(patsubst %_clean,%,$(@)) fullclean
	@echo "Finished"


$(TARGS) :: fix-crlf
	@$(MAKE) $(MAKE_ARGS) TARGET=$(@)
	@[ $$? -eq 0 ] && ./scripts/parse_dcl.pl $(@) $(BUILDNUM)
	@[ $$? -eq 0 ] && $(MAKE) -r -C builds/$(@) -j"$$NUMBER_OF_PROCESSORS" $(PTARGET)

% ::
	@echo "Target $(@) not found in targets.mk or custom_targets.mk"
	@exit 1

ifeq (,$(findstring cygwin,$(OSTYPE)))
# some files end up with \r\n instead of just \n; most unix utilities
# (cat, less, make, gcc) can handle this, others (perl, sh) can't. assume
# that 
fix-crlf ::
	@if [ -f crlf_files ]; then       \
	  perl -npe 's/\r\n/\n/' < scripts/fix_crlf.pl > scripts/fix_crlf_fixed.pl ;\
	  chmod --reference=scripts/fix_crlf.pl scripts/fix_crlf_fixed.pl          ;\
	  mv scripts/fix_crlf_fixed.pl scripts/fix_crlf.pl ;\
	  scripts/fix_crlf.pl crlf_files ;\
	  for i in `cat crlf_files`; do   \
	    scripts/fix_crlf.pl $$i      ;\
	  done ;\
	  rm -f crlf_files ;\
	 fi
else
fix-crlf :: ;
endif
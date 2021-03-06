dnl aclocal.m4 generated automatically by aclocal 1.4

dnl Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.

dnl Process this file with aclocal to get an aclocal.m4 file. Then
dnl process that with autoconf.
dnl ====================================================================
dnl
dnl     acinclude.m4
dnl
dnl     Host side implementation of the eCos infrastructure.
dnl
dnl ====================================================================
dnl####COPYRIGHTBEGIN####
dnl                                                                         
dnl ----------------------------------------------------------------------------
dnl Copyright (C) 1998, 1999, 2000 Red Hat, Inc.
dnl
dnl This file is part of the eCos host tools.
dnl
dnl This program is free software; you can redistribute it and/or modify it 
dnl under the terms of the GNU General Public License as published by the Free 
dnl Software Foundation; either version 2 of the License, or (at your option) 
dnl any later version.
dnl 
dnl This program is distributed in the hope that it will be useful, but WITHOUT 
dnl ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
dnl FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
dnl more details.
dnl 
dnl You should have received a copy of the GNU General Public License along with
dnl this program; if not, write to the Free Software Foundation, Inc., 
dnl 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
dnl
dnl ----------------------------------------------------------------------------
dnl                                                                          
dnl####COPYRIGHTEND####
dnl ====================================================================
dnl#####DESCRIPTIONBEGIN####
dnl
dnl Author(s):	bartv
dnl Contact(s):	bartv
dnl Date:	1998/07/14
dnl Version:	0.01
dnl
dnl####DESCRIPTIONEND####
dnl ====================================================================

dnl Access shared macros.
dnl AM_CONDITIONAL needs to be mentioned here or else aclocal does not
dnl incorporate the macro into aclocal.m4
sinclude(../acinclude.m4)


dnl ====================================================================
dnl Look for a 64 bit data type. It is necessary to check both C and C++
dnl compilers.
dnl
dnl A better implementation would check whether or not AC_PROG_CC and
dnl AC_PROG_CXX have been invoked and only test the appropriate
dnl compiler.
dnl
dnl When cross-compiling, default to long long on the assumption that
dnl gcc/g++ must be used and long long is likely to be the 64 bit data
dnl type. This is not guaranteed, but sufficiently likely to meet
dnl the requirements for the time being. The CHECK_SIZEOF() macro
dnl might be another way to get the desired information.

AC_DEFUN(CYG_AC_TYPE_64bit, [
    AC_REQUIRE([AC_PROG_CC])
    AC_REQUIRE([AC_PROG_CXX])

    AC_CACHE_CHECK("for a 64 bit data type",cyg_ac_cv_type_64bit,[
        for type in "long" "long long" "__int64"; do
            AC_LANG_SAVE
	    AC_LANG_C
            AC_TRY_RUN([
                main() {
                    return 8 != sizeof($type);
                }
	    ],ctype_64bit=$type,ctype_64bit="unknown",ctype_64bit="long long")
	    AC_LANG_CPLUSPLUS
            AC_TRY_RUN([
                int main(int argc, char ** argv) {
                    return 8 != sizeof($type);
                }
	    ],cxxtype_64bit=$type,cxxtype_64bit="unknown",cxxtype_64bit="long long")
	    AC_LANG_RESTORE
            if test "${ctype_64bit}" = "${type}" -a "${cxxtype_64bit}" = "${type}"; then
                cyg_ac_cv_type_64bit="${type}"
                break
            fi
        done
    ])
    if test "${cyg_ac_cv_type_64bit}" = ""; then
        AC_MSG_ERROR(Unable to figure out how to do 64 bit arithmetic)
    else
        if test "${cyg_ac_cv_type_64bit}" != "long long"; then
            AC_DEFINE_UNQUOTED(cyg_halint64,${cyg_ac_cv_type_64bit})
            AC_DEFINE_UNQUOTED(cyg_halcount64,${cyg_ac_cv_type_64bit})
        fi
    fi
])

dnl ====================================================================
dnl Check that both the C and C++ compilers support __PRETTY_FUNCTION__

AC_DEFUN(CYG_AC_C_PRETTY_FUNCTION,[
    AC_REQUIRE([AC_PROG_CC])
    AC_REQUIRE([AC_PROG_CXX])

    AC_CACHE_CHECK("for __PRETTY_FUNCTION__ support",cyg_ac_cv_c_pretty_function,[
        AC_LANG_SAVE
        AC_LANG_C
        AC_TRY_LINK(
            [#include <stdio.h>],
            [puts(__PRETTY_FUNCTION__);],
            c_ok="yes",
            c_ok="no"
        )
        AC_LANG_CPLUSPLUS
        AC_TRY_LINK(
            [#include <cstdio>],
            [puts(__PRETTY_FUNCTION__);],
            cxx_ok="yes",
            cxx_ok="no"
        )
        AC_LANG_RESTORE
        if test "${c_ok}" = "yes" -a "${cxx_ok}" = "yes"; then
            cyg_ac_cv_c_pretty_function="yes"
        fi
    ])
    if test "${cyg_ac_cv_c_pretty_function}" = "yes"; then
        AC_DEFINE(CYGDBG_INFRA_DEBUG_FUNCTION_PSEUDOMACRO)
    fi
])





# Define a conditional.

AC_DEFUN(AM_CONDITIONAL,
[AC_SUBST($1_TRUE)
AC_SUBST($1_FALSE)
if $2; then
  $1_TRUE=
  $1_FALSE='#'
else
  $1_TRUE='#'
  $1_FALSE=
fi])

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 1

dnl Usage:
dnl AM_INIT_AUTOMAKE(package,version, [no-define])

AC_DEFUN(AM_INIT_AUTOMAKE,
[AC_REQUIRE([AC_PROG_INSTALL])
PACKAGE=[$1]
AC_SUBST(PACKAGE)
VERSION=[$2]
AC_SUBST(VERSION)
dnl test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" && test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi
ifelse([$3],,
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package]))
AC_REQUIRE([AM_SANITY_CHECK])
AC_REQUIRE([AC_ARG_PROGRAM])
dnl FIXME This is truly gross.
missing_dir=`cd $ac_aux_dir && pwd`
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
AC_REQUIRE([AC_PROG_MAKE_SET])])

#
# Check to make sure that the build environment is sane.
#

AC_DEFUN(AM_SANITY_CHECK,
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "[$]*" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "[$]*" != "X $srcdir/configure conftestfile" \
      && test "[$]*" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)])

dnl AM_MISSING_PROG(NAME, PROGRAM, DIRECTORY)
dnl The program must properly implement --version.
AC_DEFUN(AM_MISSING_PROG,
[AC_MSG_CHECKING(for working $2)
# Run test in a subshell; some versions of sh will print an error if
# an executable is not found, even if stderr is redirected.
# Redirect stdin to placate older versions of autoconf.  Sigh.
if ($2 --version) < /dev/null > /dev/null 2>&1; then
   $1=$2
   AC_MSG_RESULT(found)
else
   $1="$3/missing $2"
   AC_MSG_RESULT(missing)
fi
AC_SUBST($1)])

# Add --enable-maintainer-mode option to configure.
# From Jim Meyering

# serial 1

AC_DEFUN(AM_MAINTAINER_MODE,
[AC_MSG_CHECKING([whether to enable maintainer-specific portions of Makefiles])
  dnl maintainer-mode is disabled by default
  AC_ARG_ENABLE(maintainer-mode,
[  --enable-maintainer-mode enable make rules and dependencies not useful
                          (and sometimes confusing) to the casual installer],
      USE_MAINTAINER_MODE=$enableval,
      USE_MAINTAINER_MODE=no)
  AC_MSG_RESULT($USE_MAINTAINER_MODE)
  AM_CONDITIONAL(MAINTAINER_MODE, test $USE_MAINTAINER_MODE = yes)
  MAINT=$MAINTAINER_MODE_TRUE
  AC_SUBST(MAINT)dnl
]
)

# Like AC_CONFIG_HEADER, but automatically create stamp file.

AC_DEFUN(AM_CONFIG_HEADER,
[AC_PREREQ([2.12])
AC_CONFIG_HEADER([$1])
dnl When config.status generates a header, we must update the stamp-h file.
dnl This file resides in the same directory as the config header
dnl that is generated.  We must strip everything past the first ":",
dnl and everything past the last "/".
AC_OUTPUT_COMMANDS(changequote(<<,>>)dnl
ifelse(patsubst(<<$1>>, <<[^ ]>>, <<>>), <<>>,
<<test -z "<<$>>CONFIG_HEADERS" || echo timestamp > patsubst(<<$1>>, <<^\([^:]*/\)?.*>>, <<\1>>)stamp-h<<>>dnl>>,
<<am_indx=1
for am_file in <<$1>>; do
  case " <<$>>CONFIG_HEADERS " in
  *" <<$>>am_file "*<<)>>
    echo timestamp > `echo <<$>>am_file | sed -e 's%:.*%%' -e 's%[^/]*$%%'`stamp-h$am_indx
    ;;
  esac
  am_indx=`expr "<<$>>am_indx" + 1`
done<<>>dnl>>)
changequote([,]))])


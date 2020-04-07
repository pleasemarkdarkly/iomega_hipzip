#!/bin/bash
#
# rev.sh: generate a new version for a module by making it current in vss,
#         determining and creating a new label, then sharing the new label
#         into the "stable" tree.
#         prompt the user for a changelog update and check that in
#

VSS_PROGRAM="/c/Program Files/Microsoft Visual Studio/VSS/win32/SS.EXE"

# these are all used by sed, hence the / is escaped
VSS_BASE="$\/dadio"
UNSTABLE_VSS_PROJECT="$VSS_BASE\/dev"
STABLE_VSS_PROJECT="$VSS_BASE\/stable"

# 1 if we are updating the major number
MAJOR_REV=0


#
# 0) Verify arguments
#

if [ "a$1" = "a" ]; then
  echo "Usage: $0 <vss module path> [-m] [comment]"
  exit 1
else
  MODULE=$1
fi

if [ "a$2" = "a-m" ]; then
  MAJOR_REV=1
  COMMENT=$3
else
  COMMENT=$2
fi


#
# 1) Determine the last version label applied to the module
#

# this is a pretty weak search here; we look through the history and match something
# that appears to start out with v<digit> ; you could easily label something v1saifjas
# and hose the rest of the script with it
LAST_VER=`"$VSS_PROGRAM" History $MODULE | grep "Label: \"v[0-9]*" | head -n 1 | cut -d '"' -f 2`

# if no label was found, force this to be v1_0
if [ "a$LAST_VER" = "a" ]; then
  LAST_VER="v0_9"
  MAJOR_REV=1
fi

# LAST_VER is in the format vXXX_XXX
LAST_MAJOR=`echo ${LAST_VER:1} | sed "s/_[0-9]*//g"`
LAST_MINOR=`echo $LAST_VER | sed "s/v[0-9]*_//g"`

NEW_MAJOR=
NEW_MINOR=

# strip padding
while [ ${LAST_MAJOR:0:1} -eq 0 ]; do LAST_MAJOR=${LAST_MAJOR:1}; done
while [ ${LAST_MINOR:0:1} -eq 0 ]; do LAST_MINOR=${LAST_MINOR:1}; done

if [ $MAJOR_REV -eq 1 ]; then
  NEW_MAJOR=$(($LAST_MAJOR + 1))
  NEW_MINOR=0
else
  NEW_MAJOR=$LAST_MAJOR
  NEW_MINOR=$(($LAST_MINOR + 1))

  # make sure minor numbers are zero padded properly
  # TODO should this check the padding on the minor number above ?
  if [ $NEW_MINOR -lt 10 ]; then
    NEW_MINOR="0$NEW_MINOR"
  fi
fi

NEW_VER=v"$NEW_MAJOR"_"$NEW_MINOR"

echo ""
echo "Working on module $MODULE"
echo ""
echo "Last version: $LAST_VER"
echo "New version:  $NEW_VER"
echo "Comment for label: $COMMENT"
echo ""

#
# 2) Apply the label to the module
#

# the following will apply an empty comment if none is specified on the cmd line
echo "$COMMENT" | "$VSS_PROGRAM" Label "$MODULE" -L$NEW_VER

echo ""

#
# 3) Determine where we will share the label to
#

SED_STR="s/$UNSTABLE_VSS_PROJECT/$STABLE_VSS_PROJECT/g"
STABLE_DIR=`echo $MODULE | sed $SED_STR`
MODULE_NAME=`echo ${MODULE:1} | sed "s/[a-z]*\///g"`

echo "Destination for stable share: $STABLE_DIR/$NEW_VER"
echo "Module name: $MODULE_NAME"

#
# 4) Share the label over
#

"$VSS_PROGRAM" Cp "$STABLE_DIR"
echo "" | "$VSS_PROGRAM" Share "$MODULE" -R -V$NEW_VER
echo "$COMMENT" | "$VSS_PROGRAM" Label "$STABLE_DIR" -L$NEW_VER
"$VSS_PROGRAM" Cp "$/"
"$VSS_PROGRAM" Rename "$STABLE_DIR/$MODULE_NAME" "$STABLE_DIR/$NEW_VER"

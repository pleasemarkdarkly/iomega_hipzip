#!/bin/bash
# danc@iobjects.com
# usage: ./build_dsp.sh <project name>

HEADER="header.tmpl"
FOOTER="footer.tmpl"
OUTFILE=
ORGANIZE=

function print_usage() {

  echo "Usage: ./build_dsp.sh [-o] <name>"
  echo ""
  echo "This will generate name.dsp in the current directory with all"
  echo "the files found in the current directory and all subdirectories."
  echo ""
  echo "Options:"
  echo "-o          organize, force the script to organize MSVC workspace groups"
  echo "            that mirror the filesystem groups"
  echo ""
}

if [ -z $1 ]; then
  print_usage
  exit 1
fi

if [ -z $2 ]; then
  OUTFILE="$1".dsp
else
  if [ "$1" = "-o" ]; then
    ORGANIZE=1
    OUTFILE="$1".dsp
  else
    print_usage
    exit 1
  fi
fi

cat $(dirname $0)/$HEADER | sed "s/REPLACEME/$1/g" > $OUTFILE

# generate file list
find . -type f | grep -v "tests" | egrep -i "\.cpp$|\.h$|\.hpp$|\.c$|\.S$" > res
VAR=`cat res`

CURRENT_DIR=""

echo -n "Working.../"

DONE_COUNT=0
CHAR_INDEX=0
CHAR_MAX=4
CHARS="-\|/"

for i in $VAR; do
  DN=$(dirname $i)
  if [ "$CURRENT_DIR" != "$DN" ]; then
    if [ ! -z $CURRENT_DIR ]; then
      echo "# End Group" >> $OUTFILE
    fi
    CURRENT_DIR=$DN

    GROUP_NAME=`echo ${DN:2} | sed 's/\\//_/g'`
    echo "# Begin Group \"$GROUP_NAME\""          >> $OUTFILE
    echo ""                                       >> $OUTFILE
    echo "# PROP Default_Filter \"\""             >> $OUTFILE
  fi

  echo "# Begin Source File"                      >> $OUTFILE
  echo ""                                         >> $OUTFILE
  echo "SOURCE=$(echo $i | sed "s/\\//\\\\/g")"   >> $OUTFILE
  echo "# End Source File"                        >> $OUTFILE

  # give some progress indication
  if [ $(($DONE_COUNT%10)) -eq 0 ]; then
    echo -en "\b"${CHARS:$CHAR_INDEX:1}
    CHAR_INDEX=$(($CHAR_INDEX+1))
    if [ $CHAR_INDEX -eq $CHAR_MAX ]; then CHAR_INDEX=0; fi
  fi

done

echo "# End Group"                                >> $OUTFILE

cat $(dirname $0)/$FOOTER | sed "s/REPLACEME/$1/g" >> $OUTFILE

rm res

echo -e "\nDone."
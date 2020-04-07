#!/bin/bash
#
#

DJ_PROJECT_ID=18

echo "Executing post build script" >> $LOG_FILE

# Call AddBuild.pl to put the build number in trackgear
if [ ! -z "$ADDBUILD_SCRIPT" ]; then
    $ADDBUILD_SCRIPT -p $DJ_PROJECT_ID -b dj_$BN
fi

# Call vss here to label the tree

echo "Build $BN" | $VSS Label "$VSS_ROOT_DIR/$TREE_NAME" -L"dj build $BN"

# Set up a 'current' folder in the builds directory for the latest build
# Clean up the 'current' directory
# DROP_DIR is given to us by the build script, it is where the new image was
#  just copied
# PROJECT_DIR is the location where the vss project was fetched locally

echo "Setting up the 'current' folder" >> $LOG_FILE

CURRENT_DIR=/cygdrive/e/builds/dj/current
if [ -d $CURRENT_DIR ]; then
    rm -rf $CURRENT_DIR
fi

mkdir -p $CURRENT_DIR

# copy the full build
cp $DROP_DIR/* $CURRENT_DIR

# set up the djupdate.cfg file to have the appropriate build version number
# note: this assumes that there is only one 'version' tag in the file, for the app
cat $PROJECT_DIR/djupdates.cfg | sed s/*version=[0-9]*/*version=$BN/ > $CURRENT_DIR/djupdates.cfg

# bulk copy any other files from the project tree
# dist_list is a flat file with relative filenames of files to be copied in the left
# column and destinations in the right column
echo "Copying files" >> $LOG_FILE

DIST_LIST=$PROJECT_DIR/dist_list

if [ -f $DIST_LIST ]; then
    FILE_LIST=`cat $DIST_LIST | grep -v "^#"`
    SOURCE_FILE=
    for fl in $FILE_LIST; do
        if [ -z $SOURCE_FILE ]; then
            SOURCE_FILE=$fl
        else
            DIRNAME=`dirname $fl`
            if [ -f $PROJECT_DIR/$SOURCE_FILE ]; then
                echo "Copying $PROJECT_DIR/$SOURCE_FILE to $CURRENT_DIR/$DIRNAME" >> $LOG_FILE
                mkdir -p $CURRENT_DIR/$DIRNAME
                cp -f $PROJECT_DIR/$SOURCE_FILE $CURRENT_DIR/$DIRNAME
            fi
            SOURCE_FILE=
        fi
    done
fi

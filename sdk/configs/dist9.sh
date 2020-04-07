#!/bin/bash
#
# dist9.sh perform some SDK packaging-o-love
#

# variables we inherit from the build script:
#
# BUILT_CONFIGS   the configurations that were built, space delimited
# LOG_FILE        the file to log messages to
# TARGET_NAME     name of the (build machine) target
# TREE_NAME       name of the (vss) build tree
# PTARGET         the player target(s) being built
# BN              the current build number
# DROP_DIR        the output directory
# ADDBUILD_SCRIPT the addbuilt script cmd line

# start
echo " ** $0 sdk packaging script started on $(date) **" >> $LOG_FILE

mkdir -p $DROP_DIR/player

for i in $BUILT_CONFIGS; do

    echo "** Packing configuration $i **" >> $LOG_FILE

    DIST_LIST=$PROJECT_DIR/builds/$i/_dist_list

    if [ -f $DIST_LIST ]; then
        for z in `cat $DIST_LIST`; do

            # locate the source file
            #  the source file can either be in the build or the
            #  player dir
            if [ -f "$PROJECT_DIR/builds/$i/$z" ]; then
                SRC_PATH="$PROJECT_DIR/builds/$i/$z"
            elif [ -f "$PROJECT_DIR/player/$z" ]; then
                SRC_PATH="$PROJECT_DIR/player/$z"
            else
                echo "** Unable to locate file $z (target $i) **" >> $LOG_FILE
                exit 1
            fi

            # split the file into path and filename
            #  it's important to use $z here insted of $SRC_PATH
            DIRNAME=`dirname $z`
            BASENAME=`basename $z`

            # perform some funky handling on libs
            LEN=${#BASENAME}
            EXT=${BASENAME:$(($LEN-2))}

            if [ $EXT = ".a" ]; then
                DEST_PATH=$DROP_DIR/player/$DIRNAME/libs
            else
                DEST_PATH=$DROP_DIR/player/$DIRNAME
            fi

            # create the destination directory
            mkdir -p $DEST_PATH

            # and copy the actual file
            if [ -f $DEST_PATH/$BASENAME ]; then
                echo " ** Duplicate file found: $DEST_PATH/$BASENAME exists" >> $LOG_FILE
   #            diff $z $DEST_PATH/$BASENAME > /dev/null 2&>1
   #            if [ $? -ne 0 ]; then
   #                echo "** Copying $z:" >> $LOG_FILE
   #                echo "   Destination $DEST_PATH/$BASENAME exists and files differ" >> $LOG_FILE
   #                exit 1
   #            fi
            fi
            cp -f $SRC_PATH $DEST_PATH/$BASENAME

        done
    fi
done

# perform bulk copy
if [ -f $PROJECT_DIR/dist_list ]; then
    echo "** Copying files from file listing $PROJECT_DIR/dist_list" >> $LOG_FILE

    FILE_LIST=`cat $PROJECT_DIR/dist_list | grep -v "^#"`

    echo "** File listing $FILE_LIST **" >> $LOG_FILE
    for fl in $FILE_LIST; do
        echo " First one is $PROJECT_DIR/$fl " >> $LOG_FILE
        DIRNAME=`dirname $fl`
        if [ -f $PROJECT_DIR/$fl ]; then
            for i in `cat $PROJECT_DIR/$fl | grep -v "^#"`; do
                if [ -f $PROJECT_DIR/$DIRNAME/$i ]; then
                    DN=`dirname $DIRNAME/$i`
                    mkdir -p $DROP_DIR/$DN
                    cp -f $DIRNAME/$i $DROP_DIR/$DN
                fi
            done
        fi
    done
fi

# custom step to copy dcl files. internally, we store the exposeable dcl files
# as <name>_.dcl. so here we will find all those dcl files and copy them over,
# stripping that trailing _ as we go
DCL_LIST=`cd $PROJECT_DIR/player && find . -name "*_.dcl"`

for i in $DCL_LIST; do
    TARGET_PATH=$DROP_DIR/player
    TARGET_NAME=`echo $i | sed s/\_\.dcl/\.dcl/g`

#    echo "Copying $PROJECT_DIR/player/$i to $TARGET_PATH/$TARGET_NAME"
    PTH=`dirname $TARGET_PATH/$TARGET_NAME`
    if [ -d $PTH ]; then
        cp -f $PROJECT_DIR/player/$i $TARGET_PATH/$TARGET_NAME
    fi
done

# perform zip

cd $DROP_DIR
chmod -R a+w * >> $LOG_FILE 2>&1
zip -R build_$BN "*" -x *.log -x _config.mk -x _modules.h >> $LOG_FILE 2>&1
cd -

# generate installer
if [ -f $PROJECT_DIR/installer/make_install.sh ]; then
    echo "** Generating SDK installer for build $BN **" >> $LOG_FILE
    cp "$DROP_DIR/build_$BN".zip $PROJECT_DIR/installer/iobjects_files/ >> $LOG_FILE 2>&1
    cd $PROJECT_DIR/installer
    source ./make_install.sh >> $LOG_FILE 2>&1
    cp images/installer.exe $DROP_DIR/SDK_installer_build_"$BN".exe
    cd -
fi

echo "** $0 sdk packaging script finished on $(date) **" >> $LOG_FILE

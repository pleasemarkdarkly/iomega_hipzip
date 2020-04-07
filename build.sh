#!/bin/bash
#
# build.sh: build script for new build system
# danc@iobjects.com 09/17/01
# (c) Interactive Objects
#

###########################################
#
# DO NOT MODIFY THIS FILE
#
###########################################

###########################################
# Functions
###########################################

ERROR_CODE=
ERROR_STRING=
LOCK_FILE=
LOG_FILE=
VC_FILE=
BN=
TARGET_NAME=
BLAT_MAILER=

# do_status: send an email indicating build
# status, exit
function do_status() {
    DATEVAR=`date "+%D %k:%M"`

    echo -ne "$DATEVAR\t$TARGET_NAME"_"$BN\t" >> $VC_FILE

    if [ $ERROR_CODE -ne 0 ]; then
	echo -e "\tfailed ($ERROR_STRING)" >> $VC_FILE
	echo -e "$DATEVAR\n$TARGET_NAME build $BN failed ($ERROR_STRING).\n" > ./tmp.txt
    else
	echo -e "\tsucceeded ($ERROR_STRING)" >> $VC_FILE
	echo -e "$DATEVAR\n$TARGET_NAME build $BN succeeded.\n" > ./tmp.txt
    fi

    # give some context
    if [ ! -z $LOG_FILE ]; then
	if [ -f $LOG_FILE ]; then
	    tail -n 10 $LOG_FILE >> ./tmp.txt
	fi
    fi

    $BLAT_MAILER ./tmp.txt -to builds@iobjects.com -subject "$TARGET_NAME build $BN"
    rm -f ./tmp.txt
    rm -f $LOCK_FILE
}

###########################################
# 1) set up variables, find configuration
###########################################

# base of all dadio stuff
LOCAL_ROOT_DIR="c:/dadio"
# base of all dadio stuff in vss
VSS_ROOT_DIR="\$/dadio"
# where we dump finished builds
BUILD_DROP_DIR="e:/builds"
# where we keep data about various builds
BUILD_DATA_DIR="$LOCAL_ROOT_DIR/build_data"

# where is the vss command line tool
VSS="/c/PROGRA~1/MICROS~4/VSS/win32/SS.EXE"
# where is the blat mailer tool
BLAT_MAILER="/c/dadio/blat.exe"

# where is the template version control file
TMPL_VC_FILE="$BUILD_DROP_DIR/version_control.tmpl"

# check out our config listing
BUILD_CONFIGS=$LOCAL_ROOT_DIR/build_configs

if [ ! -f "$BUILD_CONFIGS" ]; then
    echo "$0: $BUILD_CONFIGS: file not found"
    exit 1
fi

CONFIG_DATA=`grep "^$1\b" $BUILD_CONFIGS`

if [ "a$CONFIG_DATA" = "a" ]; then
    echo "$0: config '$1' not found in $BUILD_CONFIGS"
    exit 1
fi

TARGET_NAME=`echo $CONFIG_DATA | cut -f 1 -d ' '`
TREE_NAME=`echo $CONFIG_DATA | cut -f 2 -d ' '`
CONFIG_NAMES=`echo $CONFIG_DATA | cut -f 3 -d ' '`
PTARGET=`echo $CONFIG_DATA | cut -f 4- -d ' '`

export VSS BLAT_MAILER TARGET_NAME TREE_NAME PTARGET

if [ ! "$TARGET_NAME" = "$1" ]; then
    echo "$0: config '$1' not found in $BUILD_CONFIGS"
    exit 1
fi


###########################################
# 2) obtain a lock, get configuration
###########################################

# Make sure we have a build dir
if [ ! -d "$BUILD_DATA_DIR" ]; then
    mkdir -p $BUILD_DATA_DIR
fi

# Lock file setup
LOCK_FILE=$BUILD_DATA_DIR/build.LOCK

if [ -f "$LOCK_FILE" ]; then
    echo "The build system is currently locked."
    exit 1
fi

touch $LOCK_FILE

# Log file setup
LOG_FILE=$BUILD_DATA_DIR/$TARGET_NAME.log

if [ -f "$LOG_FILE" ]; then
    rm -f $LOG_FILE
fi

touch $LOG_FILE

# Build number setup
BN_FILE=$BUILD_DATA_DIR/$TARGET_NAME.buildno
RAW_BN=1

if [ -f "$BN_FILE" ]; then
    RAW_BN=`cat $BN_FILE`
fi

echo $(($RAW_BN+1)) > $BN_FILE

BN=$RAW_BN

# pad the build number to 3 digits
while [ ${#BN} -lt 3 ]; do BN=0"$BN"; done

# see if the necessary script to update trackgear
# with a buildnumber is available. if so, set a var
# so sub-scripts dont have to locate it
ADDBUILD_SCRIPT="AddBuild.pl"

if [ ! -f $ADDBUILD_SCRIPT ]; then
    echo "** AddBuild script not found, ignoring" >> $LOG_FILE
    ADDBUILD_SCRIPT=
else
    ADDBUILD_SCRIPT="/c/Perl/bin/Perl.exe ../$ADDBUILD_SCRIPT"
fi

# make our output directory
DROP_DIR=$BUILD_DROP_DIR/$TARGET_NAME/$BN
mkdir -p $DROP_DIR

# put a stamp on the log file
echo "** $TARGET_NAME build $BN started on $(date) **" >> $LOG_FILE

# version control file setup
VC_FILE=$BUILD_DROP_DIR/$TARGET_NAME/version_control.txt

if [ ! -f $VC_FILE ]; then
    cp $TMPL_VC_FILE $VC_FILE
    chmod a+w $VC_FILE
fi


export ADDBUILD_SCRIPT LOG_FILE BN DROP_DIR

###########################################
# 3) grab the latest sources from VSS
###########################################

# check to see if we already have sources. if so, junk them
PROJECT_DIR=$LOCAL_ROOT_DIR/$TREE_NAME

export PROJECT_DIR VSS_ROOT_DIR TREE_NAME

if [ -d $PROJECT_DIR ]; then
    rm -rf $PROJECT_DIR >> $LOG_FILE 2>&1
fi

mkdir -p $PROJECT_DIR

cd $PROJECT_DIR

# set the working folder
$VSS workfold "$VSS_ROOT_DIR/$TREE_NAME" "$PROJECT_DIR" >> $LOG_FILE 2>&1

ERROR_CODE=$?
if [ $ERROR_CODE -ne 0 ]; then
    ERROR_STRING="Failed to set working folder for VSS"
    do_status
    exit 1
fi


# grab the snourceages
$VSS get "$VSS_ROOT_DIR/$TREE_NAME" -R -I- >> $LOG_FILE 2>&1

ERROR_CODE=$?
if [ $ERROR_CODE -ne 0 ]; then
    ERROR_STRING="Failed to get sources from VSS"
    do_status
    exit 1
fi


###########################################
# 4) build the product, save the build
###########################################

if [ "a$PTARGET" = "a" ]; then
    PTARGET=all
fi

BUILT_CONFIGS=
CONFIG_LIST=`echo $CONFIG_NAMES | sed "s/,/ /g"`

for CONFIG_NAME in $CONFIG_LIST; do
    RUN_BUILD_SCRIPT=1
    DO_BUILD_LIST=1

    # config options
    #  these must be present in the order they are parsed
    #  preceding the config name with a hyphen prevents a build
    #  post processing script from running
    if [ ${CONFIG_NAME:0:1} = "-" ]; then
	RUN_BUILD_SCRIPT=0
	CONFIG_NAME=${CONFIG_NAME:1}
    fi
    #  preceding the config name with an underscore prevents
    #  the file _build_list from being processed
    if [ ${CONFIG_NAME:0:1} = "_" ]; then
        DO_BUILD_LIST=0
        CONFIG_NAME=${CONFIG_NAME:1}
    fi

    BUILT_CONFIGS="$BUILT_CONFIGS $CONFIG_NAME"
    export BUILT_CONFIGS

    BUILD_TREE=$LOCAL_ROOT_DIR/$TREE_NAME/builds/$CONFIG_NAME

    for CUR in $PTARGET; do

	echo "** $TARGET_NAME building PTARGET=$CUR **" >> $LOG_FILE

	make $CONFIG_NAME PTARGET="$CUR" BUILDNUM="$RAW_BN" >> $LOG_FILE 2>&1

	ERROR_CODE=$?
	if [ $ERROR_CODE -ne 0 ]; then
	    ERROR_STRING="Failed to build target $CONFIG_NAME"
	    do_status
	    cp $LOG_FILE $DROP_DIR/$CONFIG_NAME"_"$BN"."log
	    exit 1
	fi

	#  _build_list - list of generated targets (for copy)

        if [ $DO_BUILD_LIST -eq 1 ]; then
            if [ -f $BUILD_TREE/_build_list ]; then
                for i in `cat $BUILD_TREE/_build_list`; do
                    FILE_PATH=`dirname $i`
                    mkdir -p $DROP_DIR/$FILE_PATH
                    cp $BUILD_TREE/$i $DROP_DIR/$i
                done
            fi
        fi
    done

    if [ $RUN_BUILD_SCRIPT -eq 1 ]; then
	BUILD_SCRIPT="$LOCAL_ROOT_DIR/$TREE_NAME/configs/$CONFIG_NAME".sh

	if [ -x $BUILD_SCRIPT ]; then
	    echo "** Executing build script $BUILD_SCRIPT **" >> $LOG_FILE
	    /bin/bash -e $BUILD_SCRIPT

	    ERROR_CODE=$?
	    if [ $ERROR_CODE -ne 0 ]; then
		ERROR_STRING="Received error code $ERROR_CODE from $BUILD_SCRIPT"
		do_status
		cp $LOG_FILE $DROP_DIR/$CONFIG_NAME"_"$BN"."log
		exit 1
	    fi
	fi
    fi

    #  _config.mk  - configuration used to generate the build
    #  _modules.h  - header with module listing
    if [ -f $BUILD_TREE/_config.mk ]; then
        cp $BUILD_TREE/_config.mk $DROP_DIR
    fi
    if [ -f $BUILD_TREE/_modules.h ]; then
        cp $BUILD_TREE/_modules.h $DROP_DIR
    fi

done


echo "** $TARGET_NAME build $BN finished on $(date) **" >> $LOG_FILE
cp $LOG_FILE $DROP_DIR/$CONFIG_NAME"_"$BN"."log

ERROR_CODE=0
ERROR_STRING="Finished with no error"

do_status

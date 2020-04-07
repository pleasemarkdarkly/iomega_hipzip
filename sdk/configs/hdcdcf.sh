#!/bin/bash
#
#

SDK_PROJECT_ID=8

#echo "ADDBUILD_SCRIPT = $ADDBUILD_SCRIPT"
#echo "LOG_FILE = $LOG_FILE"

if [ ! -z "$ADDBUILD_SCRIPT" ]; then
    echo "Executing post build script" >> $LOG_FILE
    $ADDBUILD_SCRIPT -p $SDK_PROJECT_ID -b hdcdcfdemo_$BN
fi



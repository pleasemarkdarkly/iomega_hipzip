#!/bin/bash
#
#

POGO_PROJECT_ID=13

if [ ! -z "$ADDBUILD_SCRIPT" ]; then
    echo "Executing post build script" >> $LOG_FILE
    $ADDBUILD_SCRIPT -p $POGO_PROJECT_ID -b pogo_$BN
fi


echo "Build $BN" | $VSS Label "$VSS_ROOT_DIR/$TREE_NAME" -L"pogo build $BN"

//****************************************************************************
//
// MAKEBOOT.C - Converts the boot code image into a header file for inclusion
//              into download.c
//
// Copyright (c) 1999-2001 Cirrus Logic, Inc.
//
//****************************************************************************
#include <stdio.h>

//****************************************************************************
//
// This program simply reads flasher.rom and writes it out as a C character
// array definition in flasher.h.
//
//****************************************************************************
int
main(void)
{
    FILE *pFileIn, *pFileOut;
    int iIdx;
    unsigned char ucChar;

    //
    // Open the boot code image file.
    //
    pFileIn = fopen("flasher.rom", "rb");
    if(!pFileIn)
    {
        fprintf(stderr, "Failed to open 'flasher.rom'.\n");
        return(1);
    }

    //
    // Open the output C header file.
    //
    pFileOut = fopen("flasher.h", "w");
    if(!pFileOut)
    {
        fprintf(stderr, "Failed to open 'flasher.h'.\n");
        return(1);
    }

    //
    // Put the header C code into flasher.h.
    //
    fprintf(pFileOut, "// THIS IS A GENERATED FILE...DO NOT EDIT!\n");
    fprintf(pFileOut, "char pcBoot[2048] = {\n");

    //
    // Write 2K bytes of data into the boot code header file.
    //
    for(iIdx = 0; iIdx < 2048; iIdx++)
    {
        //
        // Read a byte from the boot code image file.
        //
        if(fread(&ucChar, 1, 1, pFileIn) == 0)
        {
            //
            // We were unable to read a byte (most likely due to the end of the
            // file), so we want to write out a '0'.
            //
            ucChar = 0;
        }

        //
        // Write out this character to the header file.
        //
        fprintf(pFileOut, "%3d, ", ucChar);

        //
        // Keep the lines reasonably short.
        //
        if((iIdx & 15) == 15)
        {
            fprintf(pFileOut, "\n");
        }
    }

    //
    // See if there is more data in the file.  There should not be...
    //
    if(fread(&ucChar, 1, 1, pFileIn) != 0)
    {
        fprintf(stderr, "Error: boot code is too large.\n");
    }

    //
    // Put the trailer C code into flasher.h.
    //
    fprintf(pFileOut, "};\n");

    //
    // Close the input and output files.
    //
    fclose(pFileIn);
    fclose(pFileOut);

    //
    // Success.
    //
    return(0);
}

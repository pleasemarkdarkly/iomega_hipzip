//****************************************************************************
//
// DOWNLOAD.C - Automates the download of code into the NOR FLASH on the
//              EP72XX and EP73XX boards.
//
// Copyright (c) 1999-2001 Cirrus Logic, Inc.
//
//****************************************************************************
#include <stdio.h>
#include "flasher.h"
#include <windows.h>

//****************************************************************************
//
// lSerialPort is the serial port which is being used.
//
//****************************************************************************
HANDLE hSerialPort;

//****************************************************************************
//
// OpenPort opens the specified serial port.
//
//****************************************************************************
int
OpenPort(long lPort)
{
    char pcName[16];

    //
    // Create the device name for the given serial port.
    //
    sprintf(pcName, "COM%d", lPort);

    //
    // Open the serial port.
    //
    hSerialPort = CreateFile(pcName, GENERIC_READ | GENERIC_WRITE, 0, 0,
                             OPEN_EXISTING, 0, 0);
    if(hSerialPort == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Could not open serial port %s.\n", pcName);
        return(0);
    }

    //
    // Success.
    //
    return(1);
}

//****************************************************************************
//
// SetBaud sets the baud rate and data format of the serial port.
//
//****************************************************************************
void
SetBaud(long lRate)
{
    DCB dcb;

    //
    // Purge any pending characters in the serial port.
    //
    PurgeComm(hSerialPort, (PURGE_TXABORT | PURGE_RXABORT |
                            PURGE_TXCLEAR | PURGE_RXCLEAR));

    //
    // Fill in the device control block.
    //
    dcb.DCBlength = sizeof(DCB);
    dcb.BaudRate = lRate;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = TRUE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fAbortOnError = FALSE;
    dcb.XonLim = 0;
    dcb.XoffLim = 0;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.XonChar = 17;
    dcb.XoffChar = 19;
    dcb.ErrorChar = 0;
    dcb.EofChar = 0;
    dcb.EvtChar = 0;
    dcb.wReserved = 0;

    //
    // Set the new serial port configuration.
    //
    SetCommState(hSerialPort, &dcb);
}

//****************************************************************************
//
// SendChar sends a character to the serial port.
//
//****************************************************************************
void
SendChar(char cChar)
{
    DWORD dwLen;

    //
    // Send this character to the serial port.
    //
    WriteFile(hSerialPort, &cChar, 1, &dwLen, NULL);
}

//****************************************************************************
//
// ReceiveChar reads a character from the serial port.
//
//****************************************************************************
char
ReceiveChar(long lTimeout)
{
    COMMTIMEOUTS sTimeouts;
    char cChar;
    DWORD dwLen;

    //
    // Fill in the timeout structure based on the timeout requested for this
    // read.
    //
    sTimeouts.ReadIntervalTimeout = 0;
    sTimeouts.ReadTotalTimeoutMultiplier = 0;
    sTimeouts.ReadTotalTimeoutConstant = lTimeout;
    sTimeouts.WriteTotalTimeoutMultiplier = 0;
    sTimeouts.WriteTotalTimeoutConstant = 0;

    //
    // Set the timeout for this read.
    //
    SetCommTimeouts(hSerialPort, &sTimeouts);

    //
    // Read a character.
    //
    if(!ReadFile(hSerialPort, &cChar, 1, &dwLen, NULL))
    {
        //
        // The read failed, so set the read character to a NULL.
        //
        cChar = 0;
    }

    //
    // If we did not read a character, then set the character to NULL.
    //
    if(dwLen != 1)
    {
        cChar = 0;
    }

    //
    // Return the character we read.
    //
    return(cChar);
}

//****************************************************************************
//
// WaitTillEmpty waits until the serial port's output buffer is empty.
//
//****************************************************************************
void
WaitTillEmpty(void)
{
    //
    // Wait for 10ms so the output buffer can drain.
    //
    Sleep(10);
}

//****************************************************************************
//
// WaitFor waits until a specific character is read from the serial port.
//
//****************************************************************************
void
WaitFor(char cWaitChar)
{
    char cChar;

    //
    // Wait until we read a specific character from the serial port.
    //
    while(1)
    {
        //
        // Read a character.
        //
        cChar = ReceiveChar(0);

        //
        // Stop waiting if we received the character.
        //
        if(cChar == cWaitChar)
        {
            break;
        }
    }
}

//****************************************************************************
//
// Prints out a usage message for the program.
//
//****************************************************************************
void
Usage(void)
{
    fprintf(stderr, "Usage: download {-h} {-v} {-p<port>} {-b<baud>} "
                    "<filename>\n\n");
    fprintf(stderr, "Downloads a program image into the NOR FLASH of a Cirrus "
                    "Logic EP72xx or EP73xx\n");
    fprintf(stderr, "board.  The Intel B3 (sizes 512KB through 4MB), C3 "
                    "(sizes 1MB through 8MB), and\n");
    fprintf(stderr, "J3(Strata) (sizes 4MB through 16MB) FLASH devices are "
                    "supported in either 16 or\n");
    fprintf(stderr, "32 bit wide configurations.\n\n");
    fprintf(stderr, "  -p <port>             Use the specified serial port "
                    "(default is \"1\").  Valid\n");
    fprintf(stderr, "                        values are 1 (COM1), 2 (COM2), 3 "
                    "(COM3), and 4 (COM4).\n\n");
    fprintf(stderr, "  -b <baud>             Use the specified baud rate "
                    "(default is \"115200\").\n");
    fprintf(stderr, "                        Valid values are 9600, 19200, "
                    "38400, 57600, and 115200.\n\n");
    fprintf(stderr, "  -v                    Prints the version of this "
                    "program.\n\n");
    fprintf(stderr, "  -h                    Prints this usage message.\n");
}

//****************************************************************************
//
// This program waits for the '<' character from the boot ROM, sends the boot
// code, waits for the '>' from the boot ROM, waits for the '?' from the boot
// code, changes the serial port rate (preferably to 115200), downloads the
// user data file, and then prints out progress status as the boot code writes
// the user data file to the NOR FLASH.
//
//****************************************************************************
int
main(int argc, char *argv[])
{
    long lPort = 1, lRate = 115200, lFileSize, lIdx, lLoop, lSum;
    char cChar, cFirstChar, cRateChar, cBuffer[1024];
    int bError = 0;
    FILE *pFile;

    //
    // First, set stdout to be unbuffered, so that our status messages are
    // always displayed immediately.
    //
    setbuf(stdout, NULL);

    //
    // See if there are any flags specified on the command line.
    //
    while(1)
    {
        //
        // If we are out of arguments, or this argument does not start with a
        // '-', then stop looking at command line arguments.
        //
        if((argc == 1) || (argv[1][0] != '-'))
        {
            break;
        }

        //
        // See if this argument is "-p".
        //
        if(argv[1][1] == 'p')
        {
            //
            // Get the port number from the command line.
            //
            lPort = atoi(argv[1] + 2);

            //
            // Make sure that the specified port number is valid.
            //
            if((lPort != 1) && (lPort != 2) && (lPort != 3) && (lPort != 4))
            {
                //
                // Tell the user that the port number is invalid.
                //
                fprintf(stderr, "Invalid serial port '%s'.\n\n", argv[1] + 2);

                //
                // Print the usage message.
                //
                Usage();

                //
                // We're done.
                //
                return(1);
            }
        }

        //
        // See if this argument is "-b".
        //
        else if(argv[1][1] == 'b')
        {
            //
            // Get the baud rate from the command line.
            //
            lRate = atoi(argv[1] + 2);

            //
            // Make sure that the specified baud rate is valid.
            //
            if((lRate != 9600) && (lRate != 19200) && (lRate != 38400) &&
               (lRate != 57600) && (lRate != 115200))
            {
                //
                // Tell the user that the baud rate is invalid.
                //
                fprintf(stderr, "Invalid baud rate '%s'.\n\n", argv[1] + 2);

                //
                // Print the usage message.
                //
                Usage();

                //
                // We're done.
                //
                return(1);
            }
        }

        //
        // See if this argument is "-h".
        //
        else if(argv[1][1] == 'h')
        {
            //
            // Print the usage message.
            //
            Usage();

            //
            // We're done.
            //
            return(1);
        }

        //
        // See if this argument is "-v".
        //
        else if(argv[1][1] == 'v')
        {
            //
            // Print the version of this program.
            //
            printf("EP72xx/EP73xx Download Version 2.00 for WIN32.\n");
            printf("Copyright (c) 1999-2001 Cirrus Logic, Inc.\n\n");
            printf("Report bugs to <epdapps@crystal.cirrus.com>.\n");

            //
            // We're done.
            //
            return(0);
        }

        //
        // An unrecognized flag was specifed.
        //
        else
        {
            //
            // Tell the user that the specified flag is invalid.
            //
            fprintf(stderr, "Invalid flag '%s'.\n\n", argv[1]);

            //
            // Print the usage message.
            //
            Usage();

            //
            // We're done.
            //
            return(1);
        }

        //
        // Skip to the next argument on the command line.
        //
        argv++;
        argc--;
    }

    //
    // Make sure that a filename was specified.
    //
    if(argc != 2)
    {
        //
        // A filename was not specified, so print the usage message.
        //
        Usage();

        //
        // We're done.
        //
        return(1);
    }

    //
    // Open the serial port to be used.
    //
    if(OpenPort(lPort) != 1)
    {
        return(1);
    }

    //
    // Open the file to be downloaded.
    //
    pFile = fopen(argv[1], "rb");
    if(!pFile)
    {
        fprintf(stderr, "Could not open file '%s'.\n", argv[1]);
        return(1);
    }

    //
    // Get the size of the file.
    //
    fseek(pFile, 0, SEEK_END);
    lFileSize = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    //
    // Round the file size up to the next 1K boundary.
    //
    lFileSize = (lFileSize + 1023) & ~1023;

    //
    // Program the initial baud rate of 9600, 8 data bits, and no parity.
    //
    SetBaud(9600);

    //
    // Tell the user to reset the board.
    //
    printf("Waiting for the board to wakeup...");

    //
    // Wait until we read a '<' from the serial port.
    //
    WaitFor('<');

    //
    // Tell the user that we are downloading the boot code.
    //
    printf("\r                                  \r");
    printf("Downloading boot code...(  0%%)");

    //
    // Write the boot code to the serial port.
    //
    for(lIdx = 0; lIdx < 2048; lIdx++)
    {
        //
        // Write this character.
        //
        SendChar(pcBoot[lIdx]);

        //
        // Periodically print out our progress.
        //
        if((lIdx & 127) == 127)
        {
            //
            // Wait until the transmit buffer is empty.
            //
            WaitTillEmpty();

            //
            // Print the updated status.
            //
            printf("\b\b\b\b\b%3d%%)", ((lIdx + 1) * 100) / 2048);
        }
    }
    printf("\r                              \r");

    //
    // Wait until we read a '>' from the serial port.
    //
    WaitFor('>');

    //
    // Wait until we read a '?' from the serial port.
    //
    printf("\n");
    while(1)
    {
        //
        // Read the next character from the serial port.
        //
        cChar = ReceiveChar(0);
        //
        // Quit waiting if this is a '?'.
        //
        if(cChar == '?')
        {
            printf("\n");
            break;
        } else {
            printf("%02x", cChar & 0xff);
        }

        //
        // See if this is a 'X'.
        //
        if(cChar == 'X')
        {
            fprintf(stderr, "The board contains an unknown FLASH.\n");
            return(1);
        }
    }

    //
    // Get the baud rate character for the given baud rate.
    //
    switch(lRate)
    {
        case 9600:
        {
            cRateChar = '0';
            break;
        }

        case 19200:
        {
            cRateChar = '1';
            break;
        }

        case 38400:
        {
            cRateChar = '2';
            break;
        }

        case 57600:
        {
            cRateChar = '3';
            break;
        }

        case 115200:
        {
            cRateChar = '4';
            break;
        }
    }

    //
    // Tell the boot code to switch to the desired baud rate.
    //
    SendChar('B');
    SendChar(cRateChar);

    //
    // Wait until the output buffer is empty.
    //
    WaitTillEmpty();

    //
    // Switch our baud rate to the desired rate.
    //
    SetBaud(lRate);

    //
    // Send a '-' character until we receive back a '?' character.
    //
    while(1)
    {
        //
        // Send a '-' character.
        //
        SendChar('-');

        //
        // Read the character.
        //
        cChar = ReceiveChar(10);

        //
        // Quit waiting if this is a '?'.
        //
        if(cChar == '?')
        {
            break;
        }
    }

    //
    // Empty out the input queue.
    //
    while((cChar = ReceiveChar(10)) != 0)
    {
    }

    //
    // Send the program FLASH command.
    //
    SendChar('F');

    //
    // Send the length of the data file.
    //
    SendChar((char)(lFileSize & 0xFF));
    SendChar((char)((lFileSize >> 8) & 0xFF));
    SendChar((char)((lFileSize >> 16) & 0xFF));
    SendChar((char)((lFileSize >> 24) & 0xFF));

    //
    // Tell the user that we are erasing the FLASH.
    //
    printf("Erasing the FLASH...");

    //
    // Wait until we receive a '!' indicating that the FLASH has been erased.
    //
    while(1)
    {
        //
        // Read the next character from the serial port.
        //
        cChar = ReceiveChar(0);

        //
        // Quit waiting if this is a '!'.
        //
        if(cChar == '!')
        {
            break;
        }

        //
        // See if this is a '&'.
        //
        if(cChar == '&')
        {
            long lFlashSize;
            lFlashSize =
                ((long) ReceiveChar(0) << 24) |
                ((long) ReceiveChar(0) << 16) |
                ((long) ReceiveChar(0) <<  8) |
                ((long) ReceiveChar(0)      );
                
            printf("\r                    \r");
            fprintf(stderr, "The image is too large for the FLASH (image = %d, flash = %d.\n", lFileSize,lFlashSize);
            return(1);
        }
    }

    //
    // Tell the user that we are downloading the file data.
    //
    printf("\rProgramming the FLASH...(  0%%)");

    //
    // Send the actual data in the file.
    //
    for(lIdx = 0; lIdx < lFileSize; lIdx += 1024)
    {
        //
        // Read the next 1K block from the file.
        //
        lLoop = fread(cBuffer, 1, 1024, pFile);

        //
        // If we could not read 1K from the file, then fill the remainder of
        // the buffer with zeros.
        //
        for(; lLoop < 1024; lLoop++)
        {
            cBuffer[lLoop] = 0;
        }

        //
        // Send this block of data until it is correctly received and
        // programmed into the FLASH.
        //
        do
        {
            //
            // Send the data for this block.
            //
            for(lLoop = 0; lLoop < 1024; lLoop++)
            {
                SendChar(cBuffer[lLoop]);
            }

            //
            // Compute the checksum for this block.
            //
            for(lLoop = 0, lSum = 0; lLoop < 1024; lLoop++)
            {
                lSum += (long)(unsigned char)cBuffer[lLoop];
            }

            //
            // Send the checksum for this block.
            //
            SendChar((char)(lSum & 0xFF));
            SendChar((char)((lSum >> 8) & 0xFF));
            SendChar((char)((lSum >> 16) & 0xFF));
            SendChar((char)((lSum >> 24) & 0xFF));

            //
            // We now need to wait to see what the target does with this block
            // of data.  Several things could happen:
            //     1) Everything is OK and it sends a '#' character.
            //     2) The block checksum was bad and it sends a '@' character.
            //     3) Some bytes were lost in the transfer and it does nothing.
            // To handle all these cases, we wait for a while to receive a
            // character.  If we never receive a character, we assume that a
            // byte was lost and start sending extra bytes, up to 16 extra
            // bytes.  The extra bytes are always 0xFF, so that the checksum
            // will always fail and the target will request a block resend.  If
            // we send 1028 extra bytes and still do not receive a reply, then
            // assume that the target died and abort the download.
            //
            for(lLoop = 0; lLoop < 16; lLoop++)
            {
                //
                // Read the character from the serial port.
                //
                cChar = ReceiveChar(250);
                if(cChar != 0)
                {
                    break;
                }

                //
                // Send a 0xFF character.
                //
                SendChar(0xFF);
            }

            //
            // If we could not get a response from the target, then indicate
            // an error and quit trying to send this and further blocks.
            //
            if(lLoop == 16)
            {
                //
                // Indicate that there was an error.
                //
                bError = 1;

                //
                // Stop trying to send this block.
                //
                break;
            }
        }
        while(cChar != '#');

        //
        // If there was an error, then quit sending data.
        //
        if(bError)
        {
            break;
        }

        //
        // Print out our progress.
        //
        printf("\b\b\b\b\b%3d%%)", ((lIdx + 1024) * 100) / lFileSize);
    }
    printf("\r                              \r");

    //
    // If there has not been an error thus far, read the final status from the
    // target.
    //
    if(!bError)
    {
        //
        // Read characters from the serial port until we receive a '*'.
        //
        while(1)
        {
            //
            // Read a character from the serial port.
            //
            cChar = ReceiveChar(0);

            //
            // If the character is a '*', then we are done.
            //
            if(cChar == '*')
            {
                printf("\n");
                break;
            }
            //
            // If the character is a '^', then there was an error programming
            // the FLASH.
            //
            if( bError ) {
                printf("%02x", cChar);
            }
            if(cChar == '^')
            {
                bError = 1;
                printf("Error writing, word 0x");
            }

        }
    }

    //
    // Close the file.
    //
    fclose(pFile);

    //
    // Tell the user we are done.
    //
    if(!bError)
    {
        printf("Successfully programmed '%s'.\n", argv[1]);
        return(0);
    }
    else
    {
        printf("Failed to program '%s'.\n", argv[1]);
        return(1);
    }
}

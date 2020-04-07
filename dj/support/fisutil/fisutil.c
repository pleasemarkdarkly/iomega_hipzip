//****************************************************************************
//
// FISUTIL.C - Automates the generation of a FIS block for a flash image
//
// Copyright (c) 1999-2001 Interactive Objects
//
//****************************************************************************
#include <stdio.h>
#include "fis.h"
#include <windows.h>

//****************************************************************************
// usage:
//
// fisutil.exe config.fis
//
// see sample config.fis for an example
//
//****************************************************************************


int iFlashSize;
unsigned long iFlashBase;
int iBlockSize;
int iImageCount;
FILE *pConfigFile;

unsigned long axtoi(char *hexStg) {
    int n = 0;         // position in string
    int m = 0;         // position in digit[] to shift
    int count;         // loop index
    unsigned long intValue = 0;  // integer value of hex string
    int digit[17];      // hold values to convert
    while (n < 16) {
        if (hexStg[n]=='\0')
            break;
        if (hexStg[n] > 0x29 && hexStg[n] < 0x40 ) //if 0 to 9
            digit[n] = hexStg[n] & 0x0f;            //convert to int
        else if (hexStg[n] >='a' && hexStg[n] <= 'f') //if a to f
            digit[n] = (hexStg[n] & 0x0f) + 9;      //convert to int
        else if (hexStg[n] >='A' && hexStg[n] <= 'F') //if A to F
            digit[n] = (hexStg[n] & 0x0f) + 9;      //convert to int
        else break;
        n++;
    }
    count = n;
    m = n - 1;
    n = 0;
    while(n < count) {
        // digit[n] is value of hex digit at position n
        // (m << 2) is the number of positions to shift
        // OR the bits into return value
        intValue = intValue | (digit[n] << (m << 2));
        m--;   // adjust the position to set
        n++;   // next digit to process
    }
    return (intValue);
}

BOOL GetNextValidLine(char *buff, int buffsize)
{
    while(fgets(buff,buffsize,pConfigFile))
    {
        if( buff[strlen(buff)-2] == '\r' )
            buff[strlen(buff)-2] = '\0';
        else buff[strlen(buff) - 1] = '\0';

        if(buff[0] == '#' || buff[0] == '\0' || buff[0] == '\r' || buff[0] == '\n')
            continue;
        else
        {
            
            //            printf("Read %x '%s'\n",buff[0],buff);
            return TRUE;
        }
    }

    return FALSE;
}


void CheckGetNext(int a)
{
    if(!a)
    {

        printf("Configuration file error\n");
        exit(1);

    }
}

static
FILE* tryopen( const char* altpath, const char* fn, const char* mode )
{
    FILE* fp = NULL;

    fp = fopen( fn, mode );

    if( fp ) return fp;
    else if( altpath ) {
        char* name = (char*) malloc( strlen(altpath)+strlen(fn)+1 );
        sprintf(name, "%s%s", altpath, fn );
        fp = fopen( name, mode );
        return fp;
    }
    else {
        return NULL;
    }
}

int
main(int argc, char *argv[])
{

    char szTemp[128];
    unsigned char *buff,*buffer_start,*flashbuffer;
    unsigned long flash_addr,flash_start;
    struct fis_image_desc *fisentry;
    int i;
    FILE *pFullFlash,*pImageFile;
    const char* altpath;
    
    if( argc == 3 ) {
        altpath = argv[2];
    } else if( argc == 2 ) {
        altpath = 0;
    } else {
        printf("Invalid parameters. \nUsage: %s <config.fis> [path]\n", argv[0]);
        exit(1);
    }

    pConfigFile = tryopen(altpath, argv[1], "r");

    if( !pConfigFile ) {
        printf("couldn't open file %s\n");
        exit(1);
    }
    
    // get basic parameters
    CheckGetNext(GetNextValidLine(szTemp,128));
    iFlashBase = axtoi(szTemp);

    CheckGetNext(GetNextValidLine(szTemp,128));
    iFlashSize = axtoi(szTemp);

    CheckGetNext(GetNextValidLine(szTemp,128));
    iBlockSize = axtoi(szTemp);

    CheckGetNext(GetNextValidLine(szTemp,128));
    iImageCount = axtoi(szTemp);

    flashbuffer = (unsigned char *)malloc(sizeof(unsigned char)*iFlashSize);
    memset((void *)flashbuffer,0xFF,iFlashSize);    


    
    flash_start = iFlashBase;

    buffer_start = (unsigned char *)flashbuffer;

    printf("Building image, base = 0x%x, size = 0x%x, blocksize = 0x%x\n",iFlashBase,iFlashSize,iBlockSize);


    for(i = 0; i < iImageCount; i++)
    {

        char szPath[64];
        unsigned long ulForceFlash;
        // zero current entry
        fisentry = (struct fis_image_desc *)(flashbuffer + (iFlashSize - iBlockSize) + sizeof(struct fis_image_desc)*i);
        memset(fisentry,0x0,sizeof(struct fis_image_desc));
    
        // get this image
        CheckGetNext(GetNextValidLine(szTemp,128));    
        strncpy(szPath,szTemp,64);

        // save name
        CheckGetNext(GetNextValidLine(szTemp,128));    
        strncpy(fisentry->name,szTemp,16);

        // save ram base
        CheckGetNext(GetNextValidLine(szTemp,128));    
        fisentry->mem_base = axtoi(szTemp);

        // save ram entry point
        CheckGetNext(GetNextValidLine(szTemp,128));    
        fisentry->entry_point = axtoi(szTemp);

        pImageFile = tryopen(altpath, szPath, "rb");

        if( !pImageFile ) {
            printf("failed to open file %s\n", szPath);
            exit(1);
        }
        
        // get file size 
        fseek(pImageFile, 0L , SEEK_END);          
        fisentry->data_length = ftell(pImageFile);
        fseek(pImageFile, 0L , SEEK_SET);


        // pad to next block boundary
        fisentry->size = fisentry->data_length + (iBlockSize - (fisentry->data_length % iBlockSize));


        // get force flash index 
        CheckGetNext(GetNextValidLine(szTemp,128));    
        ulForceFlash = axtoi(szTemp);

        if(ulForceFlash >= iFlashBase && ulForceFlash <= (iFlashBase + iFlashSize))
        {
            unsigned long adj;

            // adjust flash_start and buffer_start

            if(ulForceFlash < flash_start)
            {
                printf("Error, trying to force flash into space already used - 0x%x is < 0x%x\n",ulForceFlash,flash_start);
            }

            
            adj = ulForceFlash - flash_start;

            flash_start += adj;
            buffer_start += adj;


        }

        // update flash_start address to nearest block
        fisentry->flash_base = flash_start;
        flash_start += fisentry->size;

        // not a perfect hack, but makes it more reboot-like
        if(fisentry->mem_base == 0)
        {
            fisentry->mem_base = fisentry->flash_base;
        }

        //        printf("Writing %s as %s to 0x%x, length = %d, size = %d, ram start= 0x%x\n",szPath,fisentry->name,fisentry->flash_base,fisentry->size,fisentry->data_length,fisentry->mem_base);
        
        // Add file to the complete image
        //        printf("Writing %d bytes\n",fread(buffer_start,1,fisentry->data_length,pImageFile));
        fread(buffer_start,1,fisentry->data_length,pImageFile);
        buffer_start += fisentry->size;

        fclose(pImageFile);        

    }
    
    // put fis table entry in fis table
    //    printf("adding fis table entry\n");
    fisentry = (struct fis_image_desc *)(flashbuffer + (iFlashSize - iBlockSize) + sizeof(struct fis_image_desc)*i);
    memset(fisentry,0x0,sizeof(struct fis_image_desc));

    fisentry->flash_base = ((iFlashBase + iFlashSize) - iBlockSize);
    fisentry->mem_base = fisentry->flash_base;
    fisentry->size = iBlockSize;
    fisentry->data_length = iBlockSize;
    strcpy(fisentry->name,"FIS Directory");

    if(flash_start > ((iFlashBase + iFlashSize) - iBlockSize))
    {
        printf("fis table overwritten, too many images\n");
    }

    // Assert iImageCount > 1

    // Assert iFlashSize >= 512k
    CheckGetNext(GetNextValidLine(szTemp,128));    
    pFullFlash = tryopen(altpath, szTemp,"wb");

    if( !pFullFlash ) {
        printf("unable to open file %s\n", szTemp);
        exit(1);
    }
    printf("Writing flash image to %s\n",szTemp);
    fwrite(flashbuffer,1,iFlashSize,pFullFlash);

    fclose(pFullFlash);
    return 0;
}

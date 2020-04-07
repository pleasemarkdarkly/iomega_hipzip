// mkimg.c: generate a firmware update image from a set of images and fixed addresses
// danc@fullplaymedia.com

// todo merge common code between this and fisutil

// this is basically copied from fisutil, which reads in layouts for the entire flash
// and generates images that can be used for full burns. we dont need this; instead we
// simply need to know the images to burn out, where to burn them, etc.
// we dont need to do this; instead we just need to specif

#include <stdio.h>
#include <windows.h>
#include "imglayout.h"
#include "imgutil.h"

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
        
        //        printf("Read %x '%s'\n",buff[0],buff);

		if(buff[0] == '#' || buff[0] == '\0' || buff[0] == '\r')
			continue;
		else
		{
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

void _xor( const char* src, char* dest, int len, unsigned int val)
{
    unsigned int* s = (unsigned int*)src;
    unsigned int* d = (unsigned int*)dest;
    int i;
    
    len >>= 2; // div sizeof(unsigned int)
    for( i = 0; i < len; i++ )
    {
        d[i] = s[i] ^ val;
    }
}

int
main(int argc, char *argv[])
{

	char szTemp[128];
    char* altpath = 0;
	unsigned char *buff;
	int i, z, img_size, n, iImageCount;
    FILE* p;
    int use_xor;
    img_file_header_t* header;
    int headerlen;
    // arbitrary cap on image entries (this side only)
    char szPaths[20][64];
    fw_image_header_t img[20];

	if(argc != 4 && argc != 5 && argc != 6)
	{
		printf("Invalid parameters. \nUsage: %s <config.img> <version> <comment> <xor?> [path]\n", argv[0]);
		exit(1);
	}

    if( sscanf( argv[4], "%d", &use_xor ) == 0 || (use_xor < 0 || use_xor > 2) ) {
        printf("xor? should be '0', '1', or '2'\n");
        return -1;
    }
    
    if( argc == 6 ) {
        altpath = argv[5];
    }
    
    headerlen = sizeof(img_file_header_t) + strlen(argv[3]) + 1;
    // word align :>
    headerlen = (headerlen + 3) & ~(0x03);
    header = (img_file_header_t*) malloc( headerlen );
    strcpy( header->comment, argv[3] );
    
    if( sscanf( argv[2], "%d", &(header->version) ) == 0 ) {
        printf(" bad version\n");
        return -1;
    }
    
    memcpy( header->magic_bytes, szMagicBytes, IMG_NUM_MAGIC_BYTES );
    header->crc_offset = ( unsigned int ) (sizeof( header->crc_offset ) + &(((img_file_header_t*)0)->crc_offset));
    // firmware comes right after header
    header->fw_offset = headerlen;
    
    // at this point the magic, crc offset, fw offset, version, and comment are set
    // now to populate fw_entries, then the crc32
    
	pConfigFile = tryopen(altpath, argv[1], "r");
    if( !pConfigFile ) {
        printf(" no such file %s\n", argv[1] );
        return -1;
    }
    
    // read up the number of images here
    CheckGetNext(GetNextValidLine(szTemp,128));
    iImageCount = axtoi(szTemp);

	for(i = 0; i < iImageCount; i++)
	{

		unsigned long ulForceFlash;
	
		// get this image
		CheckGetNext(GetNextValidLine(szTemp,128));	
		strncpy(szPaths[i],szTemp,64);

        // set magic
        img[i].magic = FW_MAGIC;

        // set xor
        if( use_xor == 1 ) {
            img[i].flags = FW_CRYPT_XOR1;
        }
        else if( use_xor == 2 ) {
            img[i].flags = FW_CRYPT_XOR2;
        } else {
            img[i].flags = 0;
        }
        
		// save name
		CheckGetNext(GetNextValidLine(szTemp,128));	
		strncpy(img[i].fis_name,szTemp,16);
        img[i].fis_name[15] = 0;

		// save ram base
		CheckGetNext(GetNextValidLine(szTemp,128));
        img[i].fis_loadaddr = axtoi(szTemp);

		// save ram entry point
		CheckGetNext(GetNextValidLine(szTemp,128));
        img[i].fis_entry = axtoi(szTemp);

        // save flash address
        CheckGetNext(GetNextValidLine(szTemp,128));
        img[i].fis_burnaddr = axtoi(szTemp);

		// get file size (could probably stat)
		p = tryopen(altpath, szPaths[i], "rb");
        if( !p ) {
            printf(" failed to open file %s, quitting\n", szPaths[i] );
            return -1;
        }
		fseek(p, 0L , SEEK_END); 		 
		img[i].fis_length = ftell(p);
        fclose( p );
    }
    
    // set up offsets within the target file
    // the first offset is just after the fw_image table
    // all offsets after that are the previous offset + the length of the previous image
    img[0].offset = headerlen + (sizeof(fw_image_header_t) * i);
    for( z = 1; z < i; z++ ) {
        img[z].offset = img[z-1].offset + img[z-1].fis_length;
        // word align this data
        img[z].offset = (img[z].offset + 3) & ~0x03;
    }

    img_size = img[i-1].offset + img[i-1].fis_length;
    printf("%d images found, target file will be %d bytes\n", i, img_size );

    // allocate a buffer, move all the data in there, and write out a file
    buff = (char*) malloc( img_size );
    memcpy( buff, header, headerlen );
    n = headerlen;
    
    // nasty trick
    header = (img_file_header_t*) buff;

    // copy all the entries
    memcpy( buff + n, img, sizeof( fw_image_header_t ) * i );
    n += (sizeof( fw_image_header_t ) * i );

    header->fw_entries = i;
    
    // start dumping files in there
    for( z = 0; z < i; z++ ) {
        p = tryopen(altpath, szPaths[z], "rb");
        if( !p ) {
            printf(" failed to open file %s, quitting\n", szPaths[z] );
            return -1;
        }
        if( use_xor ) {
            unsigned int xorval = XOR_VALUE;
            char* tmp;
            if( use_xor == 2 ) {
                xorval = xorval + (z << z);
            }

            tmp = (char*) malloc( img[z].fis_length );
            fread( tmp, 1, img[z].fis_length, p );
            _xor( tmp, buff+n, img[z].fis_length, xorval );
        }
        else {
            fread( buff + n, 1, img[z].fis_length, p );
        }
        fclose( p );
        n += img[z].fis_length;

        // zero pad the file
        while( (n & 0x03) ) {
            buff[n] = 0;
            n++;
        }
    }

    // perform crc32
    header->crc32 = img_crc32( buff + header->crc_offset, n - header->crc_offset );
    
    // read up the filename
	CheckGetNext(GetNextValidLine(szTemp,128));	
	p = tryopen(altpath, szTemp,"wb");

	printf("Writing image to %s\n",szTemp);
	fwrite(buff,1,n,p);

	fclose(p);
    return 0;
}

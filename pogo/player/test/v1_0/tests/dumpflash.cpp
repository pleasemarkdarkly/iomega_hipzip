// dumpflash.cpp: dump the contents of the flash to the A: drive

#include <_modules.h>
#ifndef DDOMOD_DATASTREAM_FATFILE
#error "This test requires FAT file support"
#endif

#include <datastream/fatfile/FatFile.h>
#include <cyg/infra/diag.h>

#define LIMIT (8*1024*1024)
#define UNIT  (32*1024)
int main( int argc, char** argv ) 
{
    CFatFile file;
    int i;
    unsigned char* ptr = (unsigned char*)0xE0000000;
    
    diag_printf("%s %d enter\n", __FUNCTION__, __LINE__ );
    
    file.Open( "A:\\flash.bin" , CFatFile::WriteCreate );

    diag_printf("writing...\n");
    for( i = 0; i < LIMIT; i += UNIT ) {
        diag_printf("%d...\n", (i/UNIT));
        file.Write( (void*)(&ptr[i]), UNIT );
    }
    diag_printf("\n\ndone\n");
    
    file.Flush();

    file.Close();
    
    return 0;
}

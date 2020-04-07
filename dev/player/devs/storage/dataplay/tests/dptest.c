// dptest.c: test case for dataplay drive
// danc@iobjects.com 08/13/01
// (c) Interactive Objects

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
// #include <devs/storage/dataplay/dfs.h> // filesystem layer
#include <devs/storage/dataplay/dp.h>  // protocol layer

#define STACK_SIZE 8192*4

static char stack[STACK_SIZE];
static cyg_thread thread;
static cyg_handle_t handle;

static void thread_entry( cyg_uint32 data );

void cyg_user_start( cyg_uint32 data ) 
{
    cyg_thread_create( 10,
                       thread_entry,
                       0,
                       "thread",
                       stack,
                       STACK_SIZE,
                       &handle ,
                       &thread );


    cyg_thread_resume( handle );
    
                       
}

static void thread_entry( cyg_uint32 data ) 
{

    DEVINFOSTRUCT devinfo;
	int res; 
	diag_printf("Intializing dataplay drive\n");
    
	if(dp_init())
	{
		diag_printf("Drive Init Success\n");
	}
	else
	{
		diag_printf("Drive Init Fail\n");
		return;
	}

	dp_get_device_info(&devinfo);

	diag_printf("Device Info:\n \
		CommandRev = %2s\n  \
		ContentKeyVersion = %6s\n 
		DeviceID = %20s\n \
		DeviceType = %u \n \
		EngineNum = %20s\n \
		FirmWare = %8s\n \
		HostReadRate = %u\n \
		HostWriteRate = %u\n \
		MaxPacketSize = %u\n \
		PacketSize %u\n \
		SkipSec = %u\n \
		SpinupCurrent = %u\n \
		TimeStamp = %u\n",
		devinfo.CommandRev,
		devinfo.ContentKeyVersion,
		devinfo.DeviceID,
		devinfo.DeviceType,
		devinfo.EngineNum,
		devinfo.FirmWare,
		devinfo.HostReadRate,
		devinfo.HostWriteRate,
		devinfo.MaxPacketSize,
		devinfo.PacketSize,
		devinfo.SkipSec,
		devinfo.SpinupCurrent,
		devinfo.TimeStamp
 );

#if 0
    res = dp_power_control( DP_POWER_READY );  // spinup

    diag_printf(" dp_power_control( spinup ) = %x\n", res );


    res = dp_load_media();

    diag_printf(" dp_load_media = %x\n", res );
    
    res = dp_lock_media();

    diag_printf(" dp_lock_media = %x\n", res );

    // res = dfs_get_media_info( &nfo ) ;

    diag_printf(" dp_media_info = %x\n", res );

    diag_printf("  total_size = %d\n", nfo.total_size );
    diag_printf("  name_length = %d\n", nfo.name_length );
    diag_printf("  name = %s\n", nfo.name );

    res = dp_release_media();

    diag_printf(" dp_release_media = %x\n", res );

    res = dp_eject_media();

    diag_printf(" dp_eject_media = %x\n", res );

    diag_printf(" done\n");
    {
        void (*p)() = 0;
        p();
    }
#endif

}
    

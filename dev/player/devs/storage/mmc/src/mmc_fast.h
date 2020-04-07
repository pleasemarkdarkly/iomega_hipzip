// mmc_fast.h: function prototypes for ASM mmc routines
// Dan Conti 02/21/01 danc@iobjects.com
// (c) Interactive Objects. Tastes great, less filling

#ifndef __MMC_FAST_H__
#define __MMC_FAST_H__

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}   // brace align
#endif

// controller 0 routines
cyg_uint32  MMCExchangeData_c0( cyg_uint8 odata, cyg_uint8* idata );
void        MMCSendCommand_c0(  cyg_uint32 Arg, cyg_uint16 Cmd, cyg_uint16 crcData );
cyg_uint32  MMCGetResponse_c0(  cyg_uint8* resp_bytes, cyg_uint16 respBitLength );
cyg_uint8   MMCGetData_c0(  void );
void        MMCSendData_c0( cyg_uint8 odata );
cyg_uint32  MMCReceive_c0(  cyg_uint8* dBuf, cyg_uint16 dataLength, cyg_uint16 first );
cyg_uint32  MMCTransmit_c0( cyg_uint8* dBuf, cyg_uint16 dataLength );

// controller 1 routines
cyg_uint32  MMCExchangeData_c1( cyg_uint8 odata, cyg_uint8* idata );
void        MMCSendCommand_c1(  cyg_uint32 Arg, cyg_uint16 Cmd, cyg_uint16 crcData );
cyg_uint32  MMCGetResponse_c1(  cyg_uint8* resp_bytes, cyg_uint16 respBitLength );
cyg_uint8   MMCGetData_c1(  void );
void        MMCSendData_c1( cyg_uint8 odata );
cyg_uint32  MMCReceive_c1(  cyg_uint8* dBuf, cyg_uint16 dataLength, cyg_uint16 first );
cyg_uint32  MMCTransmit_c1( cyg_uint8* dBuf, cyg_uint16 dataLength );

#ifdef __cplusplus
};
#endif

#endif // __MMC_FAST_H__

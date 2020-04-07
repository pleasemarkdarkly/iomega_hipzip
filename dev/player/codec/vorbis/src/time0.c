/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: time backend 0 (dummy)
 last mod: $Id: time0.c,v 1.11 2001/12/20 01:00:30 segher Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "misc.h"

static vorbis_info_time *time0_unpack (vorbis_info *vi,oggpack_buffer *opb){
  return (vorbis_info_time *)"";

}
static vorbis_look_time *time0_look (vorbis_dsp_state *vd,vorbis_info_mode *mi,
                              vorbis_info_time *i){
  return (vorbis_look_time *)"";
}
static void time0_free_info(vorbis_info_time *i){
}
static void time0_free_look(vorbis_look_time *i){
}

/* export hooks */
vorbis_func_time time0_exportbundle={
  &time0_unpack,&time0_look,&time0_free_info,
  &time0_free_look
};






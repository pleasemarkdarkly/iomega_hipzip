//
// ata.h
//
//
// Copyright (c) 1998 - 2002 Fullplay Media (TM). All rights reserved
//
// ATA stress routines for EMI testing

#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/infra/testcase.h>
#include <cyg/kernel/thread.inl>

void InitATA();
void StartAudioATA();
void StartDataATA();
void StopATA();
void ata_thread(cyg_uint32 ignored);

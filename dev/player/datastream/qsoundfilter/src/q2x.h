#ifndef __Q2X_H__
#define __Q2X_H__
//****************************************************************************
//
// q2x.h 
//
//
// Copyright (c) 2000 QSound Labs, Inc.
//
//
//    History
//
//        Aug 22/2000 - MSW creation based upon Qxpander.h
//
//        Sept 5/2000 - MSW changed so that the local variables of the processor are
//                      kept within the persistant control structure
//
//        Dec 15/2000 - MSW added sample rate function and changed the size of
//                      the internal state.
//
//        Jan 28/2001 - MSW whoops the state was 79 should have been 81!
//
//****************************************************************************
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    long lState[81];
} Q2XState;

// assembly functions...
extern void q2xInitialise(Q2XState *state);
extern void q2xReset(Q2XState *state);
extern void q2xProcess(Q2XState *state, long *left, long *right);
extern void q2xSetVolume(Q2XState *state, long left, long right);
extern void q2xSetSpread(Q2XState *state, long spread);
extern void q2xSetMode(Q2XState *state, long mode);
extern void q2xSetSampleRate(Q2XState *state, long rate);

#ifdef __cplusplus
};
#endif

#endif // __Q2X_H__

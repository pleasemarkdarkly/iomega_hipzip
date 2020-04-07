//
// WaveOutKeys.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

/** \addtogroup WaveOut Wave Output */

//@{

#ifndef __WAVEOUTKEYS_H__
#define __WAVEOUTKEYS_H__

//! The key used to uniquely identify the wave output stream
//!\hideinitializer
#define WAVEOUT_KEY  0x0c

//! The ioctl key to configure hardware sample rate conversion,
//! if available.
//! \param Value Should be a pointer to an unsigned int with the
//! sample frequency of the input data
//!\hideinitializer
#define KEY_WAVEOUT_SET_SAMPLERATE  ((WAVEOUT_KEY<<8) | 0x01)

//@}

#endif // __WAVEOUTKEYS_H__

//........................................................................................
//........................................................................................
//.. File Name: id3_int28.h
//.. Last Modified By: Donni Reitz-Pesek	donni@iobjects.com
//.. Modification date: 2/17/2000
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.
//..	 All rights reserved. This code may not be redistributed in source or linkable
//.. 	 object form without the express written consent of Interactive Objects.
//.. Contact Information: www.iobjects.com
//........................................................................................
//........................................................................................
//  The authors have released ID3Lib as Public Domain (PD) and claim no copyright,
//  patent or other intellectual property protection in this work.  This means that
//  it may be modified, redistributed and used in commercial and non-commercial
//  software and hardware without restrictions.  ID3Lib is distributed on an "AS IS"
//  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.
//  
//  The ID3Lib authors encourage improvements and optimisations to be sent to the
//  ID3Lib coordinator, currently Dirk Mahoney (dirk@id3.org).  Approved
//  submissions may be altered, and will be included and released under these terms.
//  
//  Mon Nov 23 18:34:01 1998


#ifndef	ID3LIB_TYPES_28BITINT_H
#define	ID3LIB_TYPES_28BITINT_H


//#include <iostream.h>
#include <codec/mp3/id3v2/id3_types.h>


class int28
{
public:
				int28							( luint val = 0 );
				int28							( uchar *val );

uchar			operator[]						( luint posn );
//friend ostream&	operator<<						( ostream& out, int28& val );

luint			get								( void );

// *** PRIVATE INTERNAL DATA - DO NOT USE *** PRIVATE INTERNAL DATA - DO NOT USE ***
protected:
void			set								( luint val );
uchar			value[ sizeof ( luint ) ];		// the integer stored as a uchar array
};


//ostream&		operator<<						( ostream& out, int28& val );
//istream&		operator>>						( istream& in, int28& val );


#endif



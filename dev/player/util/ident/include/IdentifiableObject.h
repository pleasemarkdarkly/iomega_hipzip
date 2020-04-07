//
// IdentifiableObject.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! Dadio(tm) uses a system wide registry to allow objects and data to be
//! found and allocated by an ID instead of by key. Classes that need to be
//! accessible through this registry have a common base, in that they are
//! all uniquely identifiable. All Dadio(tm) classes that are accessible
//! through the registry derive from IdentifiableObject.

/** \addtogroup ObjectIdentification Object Identification */
//@{

#ifndef __IDENTIFIABLEOBJECT_H__
#define __IDENTIFIABLEOBJECT_H__

// an object ID has two parts: the TypeID and the ClassID

//  ObjectID = (((ObjectID) ClassID) << 16) | TypeID
typedef unsigned short TypeID;
typedef unsigned short ClassID;
typedef unsigned int ObjectID;

//! The IIdentifiableObject interface. The methods defined by
//! this interface do not need to be implemented directly ever.
struct IIdentifiableObject 
{
public:
    virtual ~IIdentifiableObject() {}

    virtual ObjectID GetObjectID() const = 0;
    virtual ClassID GetClassID()   const = 0;
    virtual TypeID GetTypeID()     const = 0;
};


//! Macro that correctly assembles an object ID given a type id and a class id
//!\arg typeID          The type ID
//!\arg classID         The class ID
//!\hideinitializer
#define ASSEMBLE_ID( typeID, classID ) (((ObjectID) typeID)<<16) | classID

//! Macro that expands to correct implementations of the
//! IIdentifiableObject routines. This macro is to be used in your definition
//! in preference to directly implementing the routines.
//!\arg typeID          The type ID
//!\arg classID         The class ID
//!\hideinitializer
#define IDENTIFY_OBJECT( typeID, classID )                                         \
        ObjectID GetObjectID() const { return ASSEMBLE_ID( typeID, classID ); }    \
        ClassID  GetClassID()  const { return classID;                        }    \
        TypeID   GetTypeID()   const { return typeID;                         }

//@}

#endif // __IDENTIFIABLEOBJECT_H__

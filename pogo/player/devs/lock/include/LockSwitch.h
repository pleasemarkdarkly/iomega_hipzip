//
// LockSwitch.h
// temancl@iobjects.com 01/02/02
// (c) Interactive Objects

#ifdef __POGO

#ifndef __LOCKSWITCH_H__
#define __LOCKSWITCH_H__


class CLockSwitch
{
public:

    //! Get a pointer to the one global instance of the lock driver
    static CLockSwitch* GetInstance();
    bool IsLocked();

private:

    CLockSwitch();
    ~CLockSwitch();

};


#endif // __LOCKSWITCH_H__
#endif // __POGO
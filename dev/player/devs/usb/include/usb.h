// toddm@iobjects.com
// (c) Interactive Objects

#ifndef __USB_H__
#define __USB_H__

#ifdef __cplusplus
extern "C" {
#endif

/* DeviceName should be a string array, zero terminated, space should be saved by the caller */
void InitializeUSB(const char * const * DeviceName);
    
#ifdef __cplusplus
};
#endif

#endif /* __USB_H__ */

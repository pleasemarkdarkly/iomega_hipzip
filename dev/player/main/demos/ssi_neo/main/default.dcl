# default.dcl: default configuration for hd, cd, and cf demo program on dharma
# edwardm@iobjects.com 8/10/01
# (c) Interactive Objects

name main
type ssi_neo

requires eventq_util

compile main.cpp Events.cpp FatHelper.cpp usb_setup.c SDKHelper.cpp

export FatHelper.h AppSettings.h usb_setup.h SDKHelper.h

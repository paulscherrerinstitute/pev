#=========================< begin file & file header >=======================
#  References
#  
#    filename : Makefile
#    author   : JFG
#    company  : IOxOS
#    creation : june 30,2008
#    version  : 0.0.1
#
#----------------------------------------------------------------------------
#  Description
#
#    This Makefile allows to build the device driver module for the PEV1100
#
#----------------------------------------------------------------------------
#  Copyright Notice
#  
#    Copyright and all other rights in this document are reserved by 
#    IOxOS Technologies SA. This documents contains proprietary information    
#    and is supplied on express condition that it may not be disclosed, 
#    reproduced in whole or in part, or used for any other purpose other
#    than that for which it is supplies, without the written consent of  
#    IOxOS Technologies SA                                                        
#
#----------------------------------------------------------------------------
#  Change History
#  $Log : $
#  
#=============================< end file header >============================
obj-m := pev.o
pev-objs := pevdrvr.o pevioctl.o pevklib.o rdwrlib.o sflashlib.o maplib.o i2clib.o vmelib.o dmalib.o histolib.o
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
EXTRA_CFLAGS := -I$(KERNELDIR)/include/xenomai -I$(KERNELDIR)/include/xenomai/posix $(ADD_CFLAGS)
PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	$(RM) *.o *.ko *.mod.c

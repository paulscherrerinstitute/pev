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
#    This Makefile allows to build the user library for the PEV1100
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
XENODIR = /usr/xenomai
INCDIRS = -I ../include 

ARCH=powerpc
#CROSS = ppc_8xx-
#CROSS_DIR = /opt/freescale/usr/local/gcc-4.3.74-eglibc-2.8.74-dp-2/powerpc-none-linux-gnuspe/bin
#CROSS = $(CROSS_DIR)/powerpc-none-linux-gnuspe-
#CROSS_DIR = /cross_tools_ppc
#CROSS_DIR = /afs/psi.ch/project/embeddedlinux/freescale/usr/local/gcc-4.3.74-eglibc-2.8.74-dp-2/powerpc-none-linux-gnuspe/bin
CROSS_DIR = /opt/eldk-5.2/powerpc-e500v2/sysroots/i686-eldk-linux/usr/bin/ppce500v2-linux-gnuspe
CROSS = $(CROSS_DIR)/powerpc-linux-gnuspe-
CC = $(CROSS)gcc
CP = $(CROSS)g++
LD = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
RANLIB = $(CROSS)ranlib


all: libpev.a libpev++.a
xeno: libpevrt.a librtdm.a libnative.a
pevulib.o:pevulib.c 
	$(CC) $(INCDIRS) $(EXTRAFLAGS) -c pevulib.c
pevulib++.o:pevulib.c 
	$(CP) $(INCDIRS) $(EXTRAFLAGS) -o pevulib++.o -c pevulib.c
pevxulib.o:pevxulib.c 
	$(CC) $(INCDIRS) $(EXTRAFLAGS) -c pevxulib.c
pevxulib++.o:pevxulib.c 
	$(CP) $(INCDIRS) $(EXTRAFLAGS) -o pevxulib++.o -c pevxulib.c
clilib.o:clilib.c 
	$(CC) $(INCDIRS) $(EXTRAFLAGS) -c clilib.c
tstlib.o:tstlib.c 
	$(CC) $(INCDIRS) $(EXTRAFLAGS) -c tstlib.c
tstxlib.o:tstxlib.c 
	$(CC) $(INCDIRS) -c tstxlib.c
libpev.a:pevulib.o pevxulib.o clilib.o tstlib.o tstxlib.o
	$(AR) r libpev.a pevulib.o pevxulib.o clilib.o tstlib.o tstxlib.o
	ranlib libpev.a
libpev++.a:pevulib++.o pevxulib++.o
	$(AR) r libpev++.a pevulib++.o pevxulib++.o
	ranlib libpev++.a
pevrtlib.o:pevrtlib.c 
	$(CC) $(INCDIRS) -I $(XENODIR)/include $(EXTRAFLAGS) -c pevrtlib.c
libpevrt.a:pevrtlib.o 
	$(AR) r libpevrt.a pevrtlib.o
	ranlib libpevrt.a
librtdm.a:
	cp $(XENODIR)/lib/librtdm.a .

libnative.a:
	cp $(XENODIR)/lib/libnative.a .

pevulib.c: ../include/pevulib.h
pevxulib.c: ../include/pevxulib.h
pevrtlib.c: ../include/pevrtlib.h
clilib.c: ../include/cli.h
clean:
	$(RM) *.o *.a

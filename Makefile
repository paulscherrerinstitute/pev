include /ioc/tools/driver.makefile

#EPICS_VERSIONS = 3.14.8
EXCLUDE_VERSIONS=3.13

SOURCES = pevDrv.c i2cDrv.c ifcDev.c 
#devLibVMEOSD.c devLibInitiator.cc
BUILDCLASSES = Linux
USR_CPPFLAGS = -DPPC -mlongcall -g -DLITTLE_ENDIAN -DDEBUG -Wall

# find the pev librarx from IOxOS
PEVDIR_embeddedlinux-e500v2 = /net/gfa-eldk/export/home/anicic/ELDK-my/driver/G/DRV/pev
#PEVDIR_SL5-x86 = ....
#PEVDIR_embeddedlinux-e500v2 = ../
USR_INCLUDES += -I$(PEVDIR_$(T_A))/include
USR_LDFLAGS  += -L$(PEVDIR_$(T_A))/lib
USR_LIBS     += pev

CROSS_COMPILER_TARGET_ARCHS=embeddedlinux-e500v2

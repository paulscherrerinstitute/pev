include /ioc/tools/driver.makefile

BUILDCLASSES = Linux
ARCH_FILTER = %-e500v2

SOURCES += pev.c
SOURCES += pevMap.c
SOURCES += pevInterrupt.c
SOURCES += pevDma.c  
SOURCES += devLibVMEOSD.c
SOURCES += pevRegDev.c
SOURCES += i2cDrv.c
SOURCES += ifcDev.c

# find the pev librarx from IOxOS
PEVDIR_embeddedlinux-e500v2 = /opt/eldk-5.2/ifc
PEVDIR_eldk52-e500v2 = /opt/eldk-5.2/ifc
#PEVDIR_SL5-x86 = ....
#PEVDIR_embeddedlinux-e500v2 = ../
PEVDIR=$(PEVDIR_$(T_A))
USR_INCLUDES += -I$(PEVDIR)/include
USR_LDFLAGS  += -L$(PEVDIR)/lib
USR_LIBS     += pev

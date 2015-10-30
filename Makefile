TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#=============================
# Build the IOC support library

# find the pev library from IOxOS
PEVDIR = /opt/eldk-5.2/ifc
USR_INCLUDES += -I$(PEVDIR)/include
USR_LDFLAGS += -L$(PEVDIR)/lib
LIB_SYS_LIBS += pev


CROSS_COMPILER_TARGET_ARCHS=eldk52-e500v2

ifeq ($(T_A),eldk52-e500v2)
LOADABLE_LIBRARY = pev
endif

DBD = pev.dbd

# allow for loadable module
pev_SRCS += pev_registerRecordDeviceDriver.cpp

# the core stuff (required)
pev_SRCS += pevDrv.c
pev_DBD  += pevDrv.dbd
pev_SRCS += pevMap.c
pev_DBD  += pevMap.dbd
pev_SRCS += pevInterrupt.c
pev_DBD  += pevInterrupt.dbd
pev_SRCS += pevDma.c  
pev_DBD  += pevDma.dbd
pev_SRCS += symbolname.cc
pev_SRCS += keypress.c

# VME master support (useful)
pev_SRCS += pevDevLib.c
pev_DBD  += pevDevLib.dbd

# VME slave configuration (optional)
pev_SRCS += pevSlaveWindow.c
pev_DBD  += pevSlaveWindow.dbd

# more device supports (optional)
pev_SRCS += ifcDev.c
pev_DBD += base.dbd
pev_DBD  += ifc.dbd
# if we have REGDEV in the RELEASE file...
ifdef REGDEV
pev_SRCS += pevRegDev.c
pev_DBD  += pevRegDev.dbd
pev_SRCS += i2cDrv.c
pev_DBD  += i2c.dbd
pev_SRCS += pevCsrRegDev.c
pev_DBD  += pevCsrRegDev.dbd
pev_LIBS += regDev
endif

INC += pev.h

include $(TOP)/configure/RULES

include /ioc/tools/driver.makefile

BUILDCLASSES = Linux
EXCLUDE_VERSIONS = 3.13 3.14.8
ARCH_FILTER = %-e500v2

SOURCES += pevDrv.c
DBDS    += pevDrv.dbd
SOURCES += pevMap.c
DBDS    += pevMap.dbd
SOURCES += pevInterrupt.c
DBDS    += pevInterrupt.dbd
SOURCES += pevDma.c  
DBDS    += pevDma.dbd
SOURCES += pevDevLib.c
DBDS    += pevDevLib.dbd
SOURCES += ifcDev.c
DBDS    += ifc.dbd
SOURCES += pevSlaveWindow.c
DBDS    += pevSlaveWindow.dbd
SOURCES += symbolname.cc
SOURCES += keypress.c

#requires regDev
SOURCES += pevRegDev.c
DBDS    += pevRegDev.dbd
SOURCES += i2cDrv.c
DBDS    += i2c.dbd
SOURCES += pevCsrRegDev.c
DBDS    += pevCsrRegDev.dbd

HEADERS += pev.h
# find the pev library from IOxOS
PEVDIR = /opt/eldk-5.2/ifc
USR_INCLUDES += -I$(PEVDIR)/include
USR_LDFLAGS += -L$(PEVDIR)/lib
LIB_LIBS += :libpev.a

HEADERS += $(PEVDIR)/include/pevioctl.h
HEADERS += $(PEVDIR)/include/pevulib.h
HEADERS += $(PEVDIR)/include/pevxulib.h

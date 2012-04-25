include /ioc/tools/driver.makefile

#EPICS_VERSIONS = 3.14.8
SOURCES = pevDrv.c lib/clilib.c lib/pevulib.c i2cDrv.c
BUILDCLASSES = Linux
USR_CPPFLAGS = -I../include -DX86_32

#embeddedlinux-ppc405

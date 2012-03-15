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
#    This Makefile allows to build the PEV1100 test launcher XprsTst
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
LIBDIR = ../../lib
INCDIR = ../../include

OLIB = $(LIBDIR)/libpev.a
OFILES = tst_0x.o tst_1x.o tst_2x.o
HFILES = $(INCDIR)/cli.h $(INCDIR)/xprstst.h


all : XprsTst PevTst

FLAGS = -g -DLITTLE_ENDIAN -DDEBUGno 
INC = -I $(INCDIR)
LIB = -L $(LIBDIR)
HFILE = XprsTst.h

XprsTst:XprsTst.o $(OFILES) $(OLIB)
	gcc $(LIB) -o XprsTst XprsTst.o -lpev -lrt


XprsTst.o: XprsTst.c Makefile $(HFILES)
	gcc $(FLAGS) $(INC) -c XprsTst.c

PevTst.o: PevTst.c Makefile $(HFILES) tstlist.h
	gcc $(FLAGS) $(INC) -c PevTst.c

PevTst:PevTst.o $(OFILES)  $(OLIB)
	gcc $(LIB) -o PevTst PevTst.o  $(OFILES) -lpev -lrt

tst_0x.o: tst_0x.c Makefile $(HFILES)
	gcc $(FLAGS) $(INC) -c tst_0x.c

tst_1x.o: tst_1x.c Makefile $(HFILES)
	gcc $(FLAGS) $(INC) -c tst_1x.c

tst_2x.o: tst_2x.c Makefile $(HFILES)
	gcc $(FLAGS) $(INC) -c tst_2x.c

clean:
	$(RM) *.o XprsTst PevTst

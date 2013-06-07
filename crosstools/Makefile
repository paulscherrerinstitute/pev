#=========================< begin file & file header >=======================
#  References
#  
#    filename : Makefile
#    author   : JFG
#    company  : IOxOS
#    creation : August 20,2012
#    version  : 0.0.1
#
#----------------------------------------------------------------------------
#  Description
#
#    This Makefile allows to build the ADNESC configuration file
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
PROJDIR = ../../..
LIBDIR = ../../lib
INCDIR = ../../include
OFILES = AdnLib.o
HFILES = a664.h
LIB = -L $(LIBDIR)
INC = -I $(INCDIR)

all : AdnConf AdnShow

FLAGS = -Wall

AdnConf:AdnConf.o $(OFILES)  $(OLIB)
	gcc $(LIB) -o AdnConf AdnConf.o $(OFILES)

AdnConf.o: AdnConf.c Makefile $(HFILES)
	gcc $(FLAGS) $(INC) -c AdnConf.c

AdnShow:AdnShow.o $(OFILES)  $(OLIB)
	gcc $(LIB) -o AdnShow AdnShow.o $(OFILES) -lrt -lm

AdnShow.o: AdnShow.c Makefile $(HFILES)
	gcc $(FLAGS) $(INC) -c AdnShow.c

AdnLib.o: AdnLib.c Makefile $(HFILES)
	gcc $(FLAGS) $(INC) -c AdnLib.c

clean:
	$(RM) *.o AdnConf AdnShow

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
#    This Makefile allows to build the PEV1100 tool set
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

OLIB = 
OFILES = 
HFILES =  

FLAGS = -g -DLITTLE_ENDIAN -DDEBUG
INC = -I $(INCDIR)
LIB = -L $(LIBDIR)

all : fsm2bin mcs2bin bin2mcs fpgabuild fpgacheck hppev

fsm2bin:fsm2bin.o $(OFILES)  $(OLIB)
	gcc $(LIB) -o fsm2bin fsm2bin.o $(OFILES)

fsm2bin.o: fsm2bin.c Makefile $(HFILES)
	gcc $(FLAGS) $(INC) -c fsm2bin.c

mcs2bin:mcs2bin.o $(OFILES)  $(OLIB)
	gcc $(LIB) -o mcs2bin mcs2bin.o $(OFILES)

mcs2bin.o: mcs2bin.c Makefile $(HFILES)
	gcc $(FLAGS) $(INC) -c mcs2bin.c

bin2mcs:bin2mcs.o $(OFILES)  $(OLIB)
	gcc $(LIB) -o bin2mcs bin2mcs.o $(OFILES)

bin2mcs.o: bin2mcs.c Makefile $(HFILES)
	gcc $(FLAGS) $(INC) -c bin2mcs.c

fpgabuild:fpgabuild.o $(OFILES)  $(OLIB)
	gcc $(LIB) -o fpgabuild fpgabuild.o $(OFILES)

fpgabuild.o: fpgabuild.c Makefile $(HFILES)
	gcc $(FLAGS) $(INC) -c fpgabuild.c

fpgacheck:fpgacheck.o $(OFILES)  $(OLIB)
	gcc $(LIB) -o fpgacheck fpgacheck.o $(OFILES)

fpgacheck.o: fpgacheck.c Makefile $(HFILES)
	gcc $(FLAGS) $(INC) -c fpgacheck.c

hppev:hppev.o $(OFILES)  $(OLIB)
	gcc $(LIB) -o hppev hppev.o $(OFILES)

hppev.o: hppev.c Makefile $(HFILES)
	gcc $(FLAGS) $(INC) -c hppev.c

install: fsm2bin mcs2bin bin2mcs fpgabuild fpgacheck hppev
	cp fsm2bin ../../bin
	cp mcs2bin ../../bin
	cp bin2mcs ../../bin
	cp fpgabuild ../../bin
	cp fpgacheck ../../bin
	cp hppev ../../bin

clean:
	$(RM) *.o *~ fsm2bin mcs2bin bin2mcs fpgabuild

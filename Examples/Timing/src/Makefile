#-------------------------------------------------------------------- 
#  
# $Author: zimoch $ 
# $Date: 2012/11/08 13:05:14 $ 
# 
# $Revision: 1.2 $ 
# $Header: /cvs/G/DRV/pev/Examples/Timing/src/Makefile,v 1.2 2012/11/08 13:05:14 zimoch Exp $ 
# 
#-------------------------------------------------------------------- 

include /ioc/tools/driver.makefile
BUILDCLASSES += Linux                               # build it only for Linux, not for vxWorks
EPICS_VERSIONS = 3.14.12                            # so far (nov. 2012) the only supported EPICS version at IFC1210 board
CROSS_COMPILER_TARGET_ARCHS = embeddedlinux-e500v2  # choose the right cross-compiler (ELDK 5.x), but only available at the *lc machines
USE_LIBVERSION = NO

getCommonFiles:
	@if (test -f COMMON_FILES);then \
		getCommons -d; \
		getCommons -c; \
        echo "done."; \
        else echo "No file COMMON_FILES"; \
    fi;
	@for i in ${SUBDIRS}; do make -C $$i getCommonFiles; done


checkCommonFilesExist:
	@if (test -f COMMON_FILES);then \
        for i in $$(grep -v '^[[:space:]]*#' COMMON_FILES | awk '{print $$1}'); do \
        echo -n ' Check common file' $$i '   -> ';\
          if [ ! -e $$i ]; \
              then echo -e "\033[31mnot here\033[0m"; \
              else echo -e "\033[32min place\033[0m"; \
          fi; \
        done; \
        else echo -e "\033[31m   ... no common files defined\033[0m"; \
    fi;

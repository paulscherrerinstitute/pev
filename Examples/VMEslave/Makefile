# $Source: /cvs/G/DRV/pev/Examples/VMEslave/Makefile,v $
# $Date: 2012/11/15 14:16:53 $
# $Revision: 1.1 $
# $Author: kalt_r $
#
# Original Author: KR84
# Date:            06.11.2012
#
# Important links:
#  - IFC1210 Codebeamer   https://codebeamer.psi.ch/cb/project/104
#  - IFC1210 User Guide   https://codebeamer.psi.ch/cb/doc/139156
#  - EPICS PEV driver     https://controls.web.psi.ch/cgi-bin/twiki/view/Main/IFC1210PEV100Driver
#
#
# Purpose: VME master and slave example for accessing TOSCA-II
#           * slot 1 system is VME master
#           * slot 2 and 3 systems are VME slaves
#
# Purpose of this file:
#           * Makefile for installation of full project to boot-pc
#             Full project means:
#             1) FPGA bitfiles for CENTRAL + IO
#             2) EPICS files


svnfiles ::
	getFiles -q -c *FPGA_BITFILES
ioc ::
	swit -V
clean ::
	getFiles -q -d *FPGA_BITFILES

MTEST-VME-KR84x ::
	getFiles -q -c MTEST-VME-KR84x_FPGA_BITFILES
	swit -ioc 'MTEST-VME-KR841 MTEST-VME-KR842 MTEST-VME-KR843'
	getFiles -q -d MTEST-VME-KR84x_FPGA_BITFILES


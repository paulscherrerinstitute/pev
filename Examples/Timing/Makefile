# $Source: /cvs/G/DRV/pev/Examples/Timing/Makefile,v $
# $Date: 2012/11/07 09:06:50 $
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
# Purpose: Timing Example for accessing TOSCA-II
#           * Provide example for VME-EVG-230 card in slot 20
#           * Provide example for VME-EVR-230RF card in slot 21
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

MTEST-PC-IFC5 ::
	getFiles -q -c MTEST-PC-IFC5_FPGA_BITFILES
	swit -ioc MTEST-PC-IFC5
	getFiles -q -d MTEST-PC-IFC5_FPGA_BITFILES


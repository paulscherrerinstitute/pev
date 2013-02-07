# $Source: /cvs/G/DRV/pev/Examples/Basic/Makefile,v $
# $Date: 2013/02/07 12:30:28 $
# $Revision: 1.3 $
# $Author: kalt_r $
#
# Original Author: KR84
# Date:            28.08.2012
#
# Important links:
#  - IFC1210 Codebeamer   https://codebeamer.psi.ch/cb/project/104
#  - IFC1210 User Guide   https://codebeamer.psi.ch/cb/doc/139156
#  - EPICS PEV driver     https://controls.web.psi.ch/cgi-bin/twiki/view/Main/IFC1210PEV100Driver
#
#
# Purpose: Basic Example for accessing TOSCA-II
#           * USERBLOCK in Virtex-6 CENTRAL FPGA
#           * 512 MB shared memory connected to Virtex-6 CENTRAL FPGA
#           * VME64x-bridge in Virtex-6 CENTRAL FPGA
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

MTEST-PC-IFC8 ::
	getFiles -q -c MTEST-PC-IFC8_FPGA_BITFILES
	swit -ioc MTEST-PC-IFC8
	getFiles -q -d MTEST-PC-IFC8_FPGA_BITFILES

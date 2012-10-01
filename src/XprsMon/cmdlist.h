/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : cmdlist.h
 *    author   : JFG
 *    company  : IOxOS
 *    creation : june 30,2008
 *    version  : 0.0.1
 *
 *----------------------------------------------------------------------------
 *  Description
 *
 *    This file contains the list of commands implemented in XprsMon
 *
 *----------------------------------------------------------------------------
 *  Copyright Notice
 *  
 *    Copyright and all other rights in this document are reserved by 
 *    IOxOS Technologies SA. This documents contains proprietary information    
 *    and is supplied on express condition that it may not be disclosed, 
 *    reproduced in whole or in part, or used for any other purpose other
 *    than that for which it is supplies, without the written consent of  
 *    IOxOS Technologies SA                                                        
 *
 *----------------------------------------------------------------------------
 *  Change History
 *
 *  $Log : $
 *
 *=============================< end file header >============================*/

#include "XprsMon.h"

int xprs_func_xxx( struct cli_cmd_para *);
int xprs_func_help( struct cli_cmd_para *);
int xprs_csr_status( struct cli_cmd_para *);

char *conf_msg[] = 
{
  "show PEV1100 configuration",
  "conf show [<device>]",
  "            all",
  "            fpga",
  "            shm",
  "            smon",
  "            static",
  "            switch",
  "            vme",
  "            pci",
  "            bmr",
0};
char *map_msg[] = 
{
  "PEV1100 address mapping operations",
  "map show <map>",
  "map clear <map>",
  "   where <map> = mas_32, mas_64, slv_vme",
0};

char *dm_msg[] = 
{
  "display data buffer from system memory",
  "dm.<ds> <addr>",
  "   where <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
0};

char *dma_msg[] = 
{
  "data transfer using DMA",
  "dma start <des_start>:<des_space>[.s] <src_start>:<src_space>[.s] <size>",
  "     where <space> =  0 -> PCIe bus address",
  "                      2 -> Shared Memory (on PEV1100, IPV1102, VCC1104, IFC1210)",
  "                      2 -> Shared Memory #1 (on MPC1200)",
  "                      3 -> Shared Memory #2 (on MPC1200)",
  "                      3 -> FPGA user area (on PEV1100, IPV1102, VCC1104)",
  "                      4 -> FPGA user area #1 (on IFC1210, MPC1210)",
  "                      5 -> FPGA user area #2 (on IFC1210, MPC1210)",
  "                      8 -> test buffer allocated in system memory",
  "                     31 -> VME SLT",
  "                     41 -> VME BLT",
  "                     51 -> VME MBLT",
  "                     61 -> VME 2eVME",
  "                     71 -> VME 2eFAST",
  "                     81 -> VME 2e160",
  "                     91 -> VME 2e233",
  "                     a1 -> VME 2e320",
  "           if .s is appended to <space>, byte swapping if performed",
0};

char *de_msg[] = 
{
  "display data buffer from P2020 ELB address space",
  "de.<ds> <addr>",
  "   where <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
0};

char *dp_msg[] = 
{
  "display data buffer from PEV1100 PCI MEM window",
  "dp.<ds> <addr>",
  "   where <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
0};

char *dr_msg[] = 
{
  "display PEV1100 registers",
  "dr.<ds> <addr>",
  "   where <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = register offset in hexadecimal ",
0};

char *ds_msg[] = 
{
  "display data buffer from PEV1100 shared memory",
  "ds<i>.<ds> <addr>",
  "   where <i> = 1,2 [shm index]",
  "         <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
0};

char *du_msg[] = 
{
  "display data buffer from FPGA user area",
  "du<i>.<ds> <addr>",
  "   where <i> = 1,2 [user area index]",
  "         <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
0};

char *dv_msg[] = 
{
  "display data buffer from VME address space",
  "dv.<ds> <addr> m:<am>",
  "   where <ds> = b,s,w,l (data size 1,2,4,8)",
  "         <addr> = address in hexadecimal ",
  "         <am> = cr, a16, a24, a32, blt",
0};

char *eeprom_msg[] = 
{ 
  "eeprom access ",
  "eeprom dump <offset> <count> ",
  "       load <offset> <file> ",
  "       verify <offset> <file> ",
  "              where <offset> = byte offset in EEPROM",
  "                    <cnt> = number of bytes to dump",
  "                    <file> = data file name for load/verify operation",
0};

char *evt_msg[] = 
{ 
  "event handling ",
  "evt wait ",
  "evt set ",
0};

char *fifo_msg[] = 
{ 
  "perform operation on communication FIFOs ",
  "fifo init ",
  "fifo.<idx> status ",
  "fifo.<idx> clear ",
  "fifo.<idx> read ",
  "fifo.<idx> write <data> ",
  "   where <idx> = fifo indexl [from 0 to 3]",
0};

char *fm_msg[]  = 
{
  "fill data buffer in system memory",
  "fm.<ds> <start..end> <data>",
  "fm.<ds> <start..end> r:<seed>",
  "fm.<ds> <start..end> s:<data>..<inc>",
  "fm.<ds> <start..end> w:<data>..<mask>",
  "fm.<ds> <start..end> f:<file>",
  "   where <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
  "         <data> = data in hexadecimal ",
  "         <seed> = seed for random generator in hexadecimal ",
  "         <inc> = increment in hexadecimal ",
  "         <mask> = mask in hexadecimal ",
  "         <file> = name of a data file ",
0};


char *fp_msg[]  = 
{
  "fill data buffer in PEV1100 PCI MEM space",
  "fp.<ds> <start..end> <data>",
  "fp.<ds> <start..end> r:<seed>",
  "fp.<ds> <start..end> s:<data>..<inc>",
  "fp.<ds> <start..end> w:<data>..<mask>",
  "fp.<ds> <start..end> f:<file>",
  "   where <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
  "         <data> = data in hexadecimal ",
  "         <seed> = seed for random generator in hexadecimal ",
  "         <inc> = increment in hexadecimal ",
  "         <mask> = mask in hexadecimal ",
  "         <am> = cr, a16, a24, a32, blt",
  "         <file> = name of a data file ",
00};

char *fpga_msg[]= 
{ 
  "fpga operation ",
  "fpga <op> <device> <para#0> <para#1> <para#2>",
  "fpga load io  file ",
  "fpga load vx6 file ",
0};

char *fs_msg[]  = 
{
  "fill data buffer in PEV1100 shared memory",
  "fs<i>.<ds> <start..end> <data>",
  "fs<i>.<ds> <start..end> r:<seed>",
  "fs<i>.<ds> <start..end> s:<data>..<inc>",
  "fs<i>.<ds> <start..end> w:<data>..<mask>",
  "fs<i>.<ds> <start..end> f:<file>",
  "   where <i> = 1,2 [shm index]",
  "         <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
  "         <data> = data in hexadecimal ",
  "         <seed> = seed for random generator in hexadecimal ",
  "         <inc> = increment in hexadecimal ",
  "         <mask> = mask in hexadecimal ",
  "         <file> = name of a data file ",
0};

char *fu_msg[]  = 
{
  "fill data buffer in PEV1100 user area",
  "fu<i>.<ds> <start..end> <data>",
  "fu<i>.<ds> <start..end> r:<seed>",
  "fu<i>.<ds> <start..end> s:<data>..<inc>",
  "fu<i>.<ds> <start..end> w:<data>..<mask>",
  "fu<i>.<ds> <start..end> f:<file>",
  "   where <i> = 1,2 [user area index]",
  "         <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
  "         <data> = data in hexadecimal ",
  "         <seed> = seed for random generator in hexadecimal ",
  "         <inc> = increment in hexadecimal ",
  "         <mask> = mask in hexadecimal ",
  "         <file> = name of a data file ",
0};

char *fv_msg[]     = 
{
  "fill data buffer in VME address space ",
  "fv.<ds> <start..end> <data> m:<am>",
  "fv.<ds> <start..end> r:<seed> m:<am>",
  "fv.<ds> <start..end> s:<data>..<inc> m:<am>",
  "fv.<ds> <start..end> w:<data>..<mask> m:<am>",
  "fv.<ds> <start..end> f:<file> m:<am>",
  "   where <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
  "         <data> = data in hexadecimal ",
  "         <seed> = seed for random generator in hexadecimal ",
  "         <inc> = increment in hexadecimal ",
  "         <mask> = mask in hexadecimal ",
  "         <file> = name of a data file ",
  "         <am> = cr, a16, a24, a32, blt",
0};

char *help_msg[]   = 
{
  "display list of commands or syntax of command <cmd> ",
  "help",
  "help <cmd>",
0};

char *i2c_msg[]     = 
{
  "perform i2c command ",
  "   i2c <dev> <op> <para0> <para1> ",
  "   i2c <dev>  cmd   <cmd>",
  "   i2c <dev>  read  <reg>",
  "   i2c <dev>  write <reg>  <data>",
  "   where <dev> = max5970",
  "                 bmr463_0",
  "                 bmr463_1",
  "                 bmr463_2",
  "                 lm95255_1",
  "                 lm95255_2",
  "                 pca9502",
  "                 ics8n3q01",
  "                 plx8624",
0};

char *ls_msg[]     = 
{ 
  "read/write loop  from/to PEV1100 shared memory ",
  "ls.<ds> <offset> [<data> [l:<loop>]]"
  "   where <ds> = b,s,w, (data size 1,2,4)",
  "         <offset> = shared memory address offset in hexadecimal ",
  "         <data> = data in hexadecimal [write cyle]",
  "         <loop> = loop count (0 -> infinite)",
0};

char *lu_msg[]     = 
{ 
  "read/write loop  from/to PEV1100 user area ",
  "lu<i>.<ds> <offset> [<data> [l:<loop>]]"
  "   where <i> = 1,2 [user area index]",
  "         <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <offset> = shared memory address offset in hexadecimal ",
  "         <data> = data in hexadecimal [write cyle]",
  "         <loop> = loop count (0 -> infinite)",
0};

char *lv_msg[]     = 
{ 
  "read/write loop  from/to VME address space ",
  "lv.<ds> <offset> <data> [l:<loop>] [m:<mode>] "
  "   where <ds> = b,s,w, (data size 1,2,4)",
  "         <offset> = shared memory address offset in hexadecimal ",
  "         <data> = data in hexadecimal [write cyle]",
  "         <loop> = loop count (0 -> infinite)",
  "         <mode> = a16, a24, a32, blt",
0};

#ifdef MPC
char *mpc_msg[] = 
{
  "perform operation on MPC board ",
  "mpc.<x> map [cr:<addr>.<size>] [a24:<addr>.<size>] [a32:<addr>.<size>]",
  "mpc.<x> i2c <dev> read <reg>",
  "mpc.<x> i2c <dev> write <reg> <data>",
  "mpc.<x> sflash id",
  "mpc.<x> sflash load SP1 <filename>",
  "mpc.<x> sflash load SP23#<i> <filename>",
  "mpc.<x> sflash load VX6#<i> <filename>",
0};
#endif

char *pc_msg[]= 
{ 
  "read/write data from/to PEV1100 PCI configuration space  ",
  "pc.<ds> <reg> <data>"
  "   where <ds> = b,s,w, (data size 1,2,4)",
  "         <reg> = register offset in hexadecimal ",
  "         <data> = data in hexadecimal [write cyle]",
0};

char *pio_msg[]= 
{ 
  "read/write data from/to PEV1100 PCI IO space  ",
  "pio.<ds> <addr> <data>"
  "   where <ds> = b,s,w, (data size 1,2,4)",
  "         <addr> = address offset in hexadecimal ",
  "         <data> = data in hexadecimal [write cyle]",
0};

char *pm_msg[]= 
{ 
  "read/write data from/to PEV1100 system memory ",
  "pm.<ds> <addr> <data>"
  "   where <ds> = b,s,w,l (data size 1,2,4,8)",
  "         <addr> = address offset in hexadecimal ",
  "         <data> = data in hexadecimal [write cyle]",
0};

char *pp_msg[]= 
{ 
  "read/write data from/to PEV1100 PCI MEM space ",
  "pp.<ds> <addr> <data>"
  "   where <ds> = b,s,w,l (data size 1,2,4,8)",
  "         <addr> = address offset in hexadecimal ",
  "         <data> = data in hexadecimal [write cyle]",
0};

char *pd_msg[] = 
{ "read/write data from/to PEV1100 device  ",
  "pd.<ds> <dev> <reg> <data>",
  "   where <ds> = b,s,w, (data size 1,2,4)",
  "         <dev> = csr, smon [device to be read/written] ",
  "         <reg> = register offset ",
  "         <data> = data in hexadecimal [write cyle]",
0};

char *pr_msg[] = 
{ "read/write data from/to PEV1100 register  ",
  "pr.<ds> <addr> <data>"
  "   where <ds> = b,s,w,l (data size 1,2,4)",
  "         <addr> = register offset in hexadecimal ",
  "         <data> = data in hexadecimal [write cyle]",
0};

char *pe_msg[] = 
{ "read/write data from/to P2020 ELB bus  ",
  "pe.<ds> <addr> <data>"
  "   where <ds> = b,s,w,l (data size 1,2,4)",
  "         <addr> = register offset in hexadecimal ",
  "         <data> = data in hexadecimal [write cyle]",
0};

char *ps_msg[]= 
{ 
  "read/write data from/to PEV1100 shared memory ",
  "ps<i>.<ds> <addr> <data>"
  "   where <i> = 1,2 [shm index]",
  "         <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
  "         <data> = data in hexadecimal [write cyle]",
0};

char *pu_msg[]= 
{ 
  "read/write data from/to FPGA user area ",
  "pu<i>.<ds> <addr> <data>"
  "   where <i> = 1,2 [user area index]",
  "         <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
  "         <data> = data in hexadecimal [write cyle]",
0};

char *pv_msg[] = 
{ 
  "read/write single data from/to VME address space ",
  "pv.<ds> <addr> <data> m:<am>" 
  "   where <ds>   = b,s,w,l (data size 1,2,4,8)",
  "         <addr> = address in hexadecimal ",
  "         <data> = data in hexadecimal [write cyle]  "
  "         <am>   = cr, a16, a24, a32, blt iack ",
0};

char *px_msg[]= 
{ 
  "read/write data from/to PEX86XX registers ",
  "px.<ds> <reg> <data>"
  "   where <ds> = b,s,w,l (data size 1,2,4,8)",
  "         <reg> = register offset in hexadecimal ",
  "         <data> = data in hexadecimal [write cyle]",
0};

char *rm_msg[] = 
{
  "read data buffer from system memory and store in file",
  "rm.<ds> <addr> f:<file>",
  "   where <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
  "         <file> = name of file to store data ",
0};

char *rmw_msg[] = 
{ 
  "VME read/modify/write ",
  "rmw.<ds> <addr> <cmp> <up> m:<am>" 
  "   where <ds>   = b,s,w,l (data size 1,2,4,8)",
  "         <addr> = VME address in hexadecimal ",
  "         <cmp > = compare data in hexadecimal [read cyle]  ",
  "         <up> = update data in hexadecimal [write cyle]  ",
  "         <am>   = 9,d,29,2d,39,3d  ",
0};

char *re_msg[] = 
{
  "display data buffer from P2020 ELB address space",
  "read data buffer from system memory and store in file",
  "re.<ds> <addr> f:<file>",
  "   where <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
  "         <file> = name of file to store data ",
0};


char *rp_msg[] = 
{
  "display data buffer from PEV interfave PCI MEM window",
  "read data buffer from system memory and store in file",
  "rp.<ds> <addr> f:<file>",
  "   where <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
  "         <file> = name of file to store data ",
0};

char *rr_msg[] = 
{
  "read PEV interface registers and store in file",
  "rr.<ds> <addr> f:<file>",
  "   where <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = register offset in hexadecimal ",
  "         <file> = name of file to store data ",
0};

char *rs_msg[] = 
{
  "read data buffer from PEV interfave shared memory and store in file",
  "rs<i>.<ds> <addr> f:<file>",
  "   where <i> = 1,2 [shm index]",
  "         <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
  "         <file> = name of file to store data ",
0};

char *ru_msg[] = 
{
  "read data buffer from FPGA user area and store in file",
  "ru<i>.<ds> <addr> f:<file>",
  "   where <i> = 1,2 [user area index]",
  "         <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <addr> = address offset in hexadecimal ",
  "         <file> = name of file to store data ",
0};

char *rv_msg[] = 
{
  "read data buffer from VME address space and store in file",
  "rv.<ds> <addr> m:<am> f:<file>",
  "   where <ds> = b,s,w,l (data size 1,2,4,8)",
  "         <addr> = address in hexadecimal ",
  "         <am> = cr, a16, a24, a32, blt",
  "         <file> = name of file to store data ",
0};

char *sflash_msg[]= 
{ 
  "sflash operation ",
  "sflash <op> <offset> <para#0> <para#1> <para#2>",
  "sflash id <dev>",
  "sflash rdsr <dev>",
  "sflash wrsr <dev> <sr>",
  "sflash dump   <off>      <len> ",
  "sflash load   <fpga>#<i> <file> ",
  "sflash load   <off>      <file> ",
  "sflash read   <fpga>#<i> <len>  <file> ",
  "sflash read   <off>      <len>  <file> ",
  "sflash sign   <fpga>#<i> ",
  "sflash check  <fpga>#<i> ",
  "   where <dev> = 0,1,2,3 [fpga device]",
  "         <fpga> = fpga, pon, io, central",
  "         <i> = 0,1,2,3 [bitstream index]",
  "         <off> = SFLASH address offset in hexadecimal ",
  "         <len> = size in bytes to transfer ",
  "         <file> = name of the to be loaded/saved ",
0};

char *sign_msg[]= 
{ 
  "board signature ",
  "sign show",
  "sign set",
0};

char *ts_msg[]  = 
{
  "perform read/write test on shared memory",
  "ts<i>.<ds> <start>..<end> <data>",
  "   where <i> = 1,2 [shm index]",
  "         <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <start> = address offse in hexadecimal of first  location",
  "         <end> = address offset in hexadecimal of last location",
  "         <data> = data in hexadecimal used for test initialization",
0};

char *tu_msg[]  = 
{
  "perform read/write test on FPGA user area",
  "tu<i>.<ds> <start>..<end> <data>",
  "   where <i> = 1,2 [user area index]",
  "         <ds> = b,s,w,l [data size 1,2,4,8]",
  "         <start> = address offse in hexadecimal of first  location",
  "         <end> = address offset in hexadecimal of last location",
  "         <data> = data in hexadecimal used for test initialization",
0};

char *tv_msg[]  = 
{
  "perform read/write test on VME bus",
  "tv.<ds> <start>..<end> <data> m:<am>",
  "   where <ds> = b,s,w,l (data size 1,2,4,8)",
  "         <start> = address offse in hexadecimal of first  location",
  "         <end> = address offset in hexadecimal of last location",
  "         <data> = data in hexadecimal used for test initialization",
  "         <am>   = cr, a16, a24, a32, blt  ",
0};

char *tinit_msg[] = 
{
  "launch test suite",
  "tinit [<testfile>]",
0};

char *tkill_msg[] = 
{
  "kill test suite",
0};

char *tlist_msg[] = 
{
  "display a list of existing test",
0};

char *tset_msg[] = 
{
  "set test control parameter",
  "tset exec=fast -> execute test in fast mode [default]",
  "tset exec=val  -> execute test in validation mode",
  "tset err=stop  -> stop test execution if error [default]",
  "tset err=cont  -> go for next test if error",
  "tset loop=<n>",
  "     where <n>   = number of time a test is execute (0->infinite)",
  "tset logfile=<filename>",
  "tset log=<op>",
  "     where <op>  = off -> close logfile if currently open",
  "                   new -> create new logfile",
  "                   add -> append to existing logfile or create logfile",
0};

char *tstart_msg[] = 
{
  "start execution of a test or a chain of tests",
  "tstart <test>",
  "tstart <start>..<end>",
0};

char *tstop_msg[] = 
{
  "stop current test execution",
0};

char *tstatus_msg[] = 
{
  "show status of test suite",
0};

char *timer_msg[] = 
{
  "perform operation on PEV1100 internal timer ",
  "timer <op> <para#1> <para#1> ",
  "timer start mode  time ",
  "timer restart ",
  "timer stop ",
  "timer read ",
0};

char *tty_msg[] = 
{
  "send string to ttyUSB0 ",
  "tty close ",
  "tty open ",
  "tty send <string> ",
0};

char *vme_msg[] = 
{
  "configure vme interface ",
  "vme conf ",
  "vme init ",
  "vme irq init ",
  "vme irq mask <irq_set> ",
  "vme irq unmask <irq_set> ",
  "vme irq alloc <irq_set>",
  "vme irq wait <timeout> ",
  "vme irq clear ",
  "vme sysreset ",
0};


//little quicklist menu CM
char *lql_msg[] =
{
  "Mapping of memory is little-endian, ex:",
  "pio.b : [3:0] -> [3:0]"
  "pio.w : [31:28][27:24]..[3:0] -> [31:28][27:24]..[3:0]",
  "pm/dm/...w  : [3:0][7:4][11:8]..[31:28] -> idem",
0};

struct cli_cmd_list cmd_list[] =
{
  { "conf"   , xprs_conf_show,    conf_msg   , 0},
  { "dm"     , xprs_rdwr_dm,      dm_msg     , 0},
  { "dma"    , xprs_rdwr_dma,     dma_msg    , 0},
  { "de"     , xprs_rdwr_dm,      de_msg     , 0},
  { "dp"     , xprs_rdwr_dm,      dp_msg     , 0},
  { "dr"     , xprs_rdwr_dm,      dr_msg     , 0},
  { "ds"     , xprs_rdwr_dm,      ds_msg     , 0},
  { "ds1"    , xprs_rdwr_dm,      ds_msg     , 0},
  { "ds2"    , xprs_rdwr_dm,      ds_msg     , 0},
  { "du"     , xprs_rdwr_dm,      du_msg     , 0},
  { "du1"    , xprs_rdwr_dm,      du_msg     , 0},
  { "du2"    , xprs_rdwr_dm,      du_msg     , 0},
  { "dv"     , xprs_rdwr_dm,      dv_msg     , 0},
/*{ "evt"    , xprs_rdwr_evt,     evt_msg    , 0},*/
  { "eeprom" , xprs_eeprom,       eeprom_msg , 0},
  { "fifo"   , xprs_fifo,         fifo_msg   , 0},
  { "fm"     , xprs_rdwr_fm,      fm_msg     , 0},
  { "fp"     , xprs_rdwr_fm,      fp_msg     , 0},
  { "fpga"   , xprs_fpga,         fpga_msg   , 0},
  { "fs"     , xprs_rdwr_fm,      fs_msg     , 0},
  { "fs1"    , xprs_rdwr_fm,      fs_msg     , 0},
  { "fs2"    , xprs_rdwr_fm,      fs_msg     , 0},
  { "fu"     , xprs_rdwr_fm,      fu_msg     , 0},
  { "fu1"    , xprs_rdwr_fm,      fu_msg     , 0},
  { "fu2"    , xprs_rdwr_fm,      fu_msg     , 0},
  { "fv"     , xprs_rdwr_fm,      fv_msg     , 0},
  { "help"   , xprs_func_help,    help_msg   , 0},
  { "i2c"    , xprs_i2c,          i2c_msg    , 0},
  { "lql"    , xprs_func_xxx,     lql_msg    , 0},
  { "ls"     , xprs_rdwr_lm,      ls_msg     , 0},
  { "ls1"    , xprs_rdwr_lm,      ls_msg     , 0},
  { "ls2"    , xprs_rdwr_lm,      ls_msg     , 0},
  { "lu"     , xprs_rdwr_lm,      lu_msg     , 0},
  { "lu1"    , xprs_rdwr_lm,      lu_msg     , 0},
  { "lu2"    , xprs_rdwr_lm,      lu_msg     , 0},
  { "lv"     , xprs_rdwr_lm,      lv_msg     , 0},
  { "map"    , xprs_map,          map_msg    , 0},
#ifdef MPC
  { "mpc"    , xprs_mpc,          mpc_msg    , 0},
#endif
  { "pc"     , xprs_rdwr_pm,      pc_msg     , 0},
  { "pe"     , xprs_rdwr_pm,      pe_msg     , 0},
  { "pio"    , xprs_rdwr_pm,      pio_msg    , 0},
  { "pm"     , xprs_rdwr_pm,      pm_msg     , 0},
  { "pp"     , xprs_rdwr_pm,      pp_msg     , 0},
  { "pr"     , xprs_rdwr_pm,      pr_msg     , 0},
  { "ps"     , xprs_rdwr_pm,      ps_msg     , 0},
  { "ps1"    , xprs_rdwr_pm,      ps_msg     , 0},
  { "ps2"    , xprs_rdwr_pm,      ps_msg     , 0},
  { "pu"     , xprs_rdwr_pm,      pu_msg     , 0},
  { "pu1"    , xprs_rdwr_pm,      pu_msg     , 0},
  { "pu2"    , xprs_rdwr_pm,      pu_msg     , 0},
  { "pv"     , xprs_rdwr_pm,      pv_msg     , 0},
  { "px"     , xprs_rdwr_pm,      px_msg     , 0},
  { "rm"     , xprs_rdwr_dm,      rm_msg     , 0},
  { "re"     , xprs_rdwr_dm,      re_msg     , 0},
  { "rp"     , xprs_rdwr_dm,      rp_msg     , 0},
  { "rr"     , xprs_rdwr_dm,      rr_msg     , 0},
  { "rs"     , xprs_rdwr_dm,      rs_msg     , 0},
  { "rs1"    , xprs_rdwr_dm,      rs_msg     , 0},
  { "rs2"    , xprs_rdwr_dm,      rs_msg     , 0},
  { "ru"     , xprs_rdwr_dm,      ru_msg     , 0},
  { "ru1"    , xprs_rdwr_dm,      ru_msg     , 0},
  { "ru2"    , xprs_rdwr_dm,      ru_msg     , 0},
  { "rv"     , xprs_rdwr_dm,      rv_msg     , 0},
  { "rmw"    , xprs_rdwr_rmw,     rmw_msg    , 0},
  { "sflash" , xprs_sflash,       sflash_msg , 0},
  { "sign"   , xprs_sign,         sign_msg   , 0},
  { "ts"     , xprs_rdwr_tm,      ts_msg     , 0},
  { "ts1"    , xprs_rdwr_tm,      ts_msg     , 0},
  { "ts2"    , xprs_rdwr_tm,      ts_msg     , 0},
  { "tu"     , xprs_rdwr_tm,      tu_msg     , 0},
  { "tu1"    , xprs_rdwr_tm,      tu_msg     , 0},
  { "tu2"    , xprs_rdwr_tm,      tu_msg     , 0},
  { "tv"     , xprs_rdwr_tm,      tv_msg     , 0},
  { "timer"  , xprs_timer,        timer_msg  , 0},
  { "tinit"  , xprs_tinit,        tinit_msg  , 0},
  { "tkill"  , xprs_tkill,        tkill_msg  , 0},
  { "tlist"  , xprs_tlist,        tlist_msg  , 0},
  { "tset"   , xprs_tset,         tset_msg   , 0},
  { "tstart" , xprs_tstart,       tstart_msg , 0},
  { "tstatus", xprs_tstatus,      tstatus_msg, 0},
  { "tstop"  , xprs_tstop,        tstop_msg  , 0},
  { "tty"    , xprs_tty,          tty_msg    , 0},
  { "vme"    , xprs_vme,          vme_msg    , 0},
  { (char *)0,} 
};

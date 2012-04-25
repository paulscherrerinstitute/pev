/*=========================< begin file & file header >=======================
 *  References
 *  
 *    filename : tstlist.h
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

int tst_01( struct tst_ctl *);
int tst_02( struct tst_ctl *);
int tst_03( struct tst_ctl *);
int tst_04( struct tst_ctl *);
int tst_08( struct tst_ctl *);
int tst_09( struct tst_ctl *);
int tst_11( struct tst_ctl *);
int tst_12( struct tst_ctl *);
int tst_13( struct tst_ctl *);
int tst_18( struct tst_ctl *);
int tst_19( struct tst_ctl *);
int tst_1a( struct tst_ctl *);
int tst_20( struct tst_ctl *);
int tst_21( struct tst_ctl *);
int tst_22( struct tst_ctl *);
int tst_23( struct tst_ctl *);
int tst_24( struct tst_ctl *);
int tst_25( struct tst_ctl *);
int tst_26( struct tst_ctl *);
int tst_27( struct tst_ctl *);
int tst_28( struct tst_ctl *);
int tst_29( struct tst_ctl *);
int tst_2a( struct tst_ctl *);
int tst_2b( struct tst_ctl *);
int tst_2c( struct tst_ctl *);
int tst_2d( struct tst_ctl *);
int tst_2e( struct tst_ctl *);
int tst_2f( struct tst_ctl *);
int tst_80( struct tst_ctl *);
int tst_81( struct tst_ctl *);
int tst_82( struct tst_ctl *);
int tst_83( struct tst_ctl *);
int tst_84( struct tst_ctl *);
int tst_85( struct tst_ctl *);
int tst_86( struct tst_ctl *);
int tst_87( struct tst_ctl *);


char *tst_01_msg[] = 
{
  "my test...",
0};
char *tst_02_msg[] = 
{
  "Test SHM: CPU access through PCI MEM",
0};
char *tst_03_msg[] = 
{
  "Test SHM: CPU access through PCI PMEM",
0};
char *tst_04_msg[] = 
{
  "Test VME: CPU access",
0};
char *tst_08_msg[] = 
{
  "Test VME: Interrupts with autovector",
0};
char *tst_09_msg[] = 
{
  "Test VME: Interrupts",
0};
char *tst_11_msg[] = 
{
  "Test DMA: KBUF -> SHM nowait mode",
0};
char *tst_12_msg[] = 
{
  "Test DMA: KBUF -> SHM polling mode",
0};
char *tst_13_msg[] = 
{
  "Test DMA: KBUF -> SHM interrupt mode",
0};
char *tst_18_msg[] = 
{
  "Test DMA: SHM -> KBUF nowait mode",
0};
char *tst_19_msg[] = 
{
  "Test DMA: SHM -> KBUF polling mode",
0};
char *tst_1a_msg[] = 
{
  "Test DMA: SHM -> KBUF interrupt mode",
0};
char *tst_20_msg[] = 
{
  "Test DMA: VME SLT -> SHM",
0};
char *tst_21_msg[] = 
{
  "Test DMA: VME BLT -> SHM",
0};
char *tst_22_msg[] = 
{
  "Test DMA: VME MBLT -> SHM",
0};
char *tst_23_msg[] = 
{
  "Test DMA: VME 2eVME -> SHM",
0};
char *tst_24_msg[] = 
{
  "Test DMA: VME 2eVME Fast -> SHM",
0};
char *tst_25_msg[] = 
{
  "Test DMA: VME 2eSST 160 -> SHM",
0};
char *tst_26_msg[] = 
{
  "Test DMA: VME 2eSST 233 -> SHM",
0};
char *tst_27_msg[] = 
{
  "Test DMA: VME 2eSST 230 -> SHM",
0};
char *tst_28_msg[] = 
{
  "Test DMA: SHM -> VME SLT",
0};
char *tst_29_msg[] = 
{
  "Test DMA: SHM -> VME BLT",
0};
char *tst_2a_msg[] = 
{
  "Test DMA: SHM -> VME MBLT",
0};
char *tst_2b_msg[] = 
{
  "Test DMA: SHM -> VME 2eVME",
0};
char *tst_2c_msg[] = 
{
  "Test DMA: SHM -> VME 2eVME Fast",
0};
char *tst_2d_msg[] = 
{
  "Test DMA: SHM -> VME 2eSST 160",
0};
char *tst_2e_msg[] = 
{
  "Test DMA: SHM -> VME 2eSST 233",
0};
char *tst_2f_msg[] = 
{
  "Test DMA: SHM -> VME 2eSST320",
0};
char *tst_80_msg[] = 
{
  "Test access to I2C devices",
0};

char *tst_81_msg[] = 
{
  "Test 81",
0};

char *tst_82_msg[] = 
{
  "Test 82",
0};

char *tst_83_msg[] = 
{
  "Test 83",
0};

char *tst_84_msg[] = 
{
  "Test 84",
0};

char *tst_85_msg[] = 
{
  "Test PMC connector IFC1210",
0};

char *tst_86_msg[] = 
{
  "Test FMC connector IFC1210",
0};

char *tst_87_msg[] = 
{
  "Test VME P2 connector IFC1210",
0};


struct tst_list
{
  int idx;
  int (* func)();
  char **msg;
  int status;
};

struct tst_list tst_list[] =
{
  { 0x01, tst_01, tst_01_msg, 0},
  { 0x02, tst_02, tst_02_msg, 0},
  { 0x03, tst_03, tst_03_msg, 0},
  { 0x04, tst_04, tst_04_msg, 0},
  { 0x08, tst_08, tst_08_msg, 0},
  { 0x09, tst_09, tst_09_msg, 0},
  { 0x11, tst_11, tst_11_msg, 0},
  { 0x12, tst_12, tst_12_msg, 0},
  { 0x13, tst_13, tst_13_msg, 0},
  { 0x18, tst_18, tst_18_msg, 0},
  { 0x19, tst_19, tst_19_msg, 0},
  { 0x1a, tst_1a, tst_1a_msg, 0},
  { 0x20, tst_20, tst_20_msg, 0},
  { 0x21, tst_21, tst_21_msg, 0},
  { 0x22, tst_22, tst_22_msg, 0},
  { 0x23, tst_23, tst_23_msg, 0},
  { 0x24, tst_24, tst_24_msg, 0},
  { 0x25, tst_25, tst_25_msg, 0},
  { 0x26, tst_26, tst_26_msg, 0},
  { 0x27, tst_27, tst_27_msg, 0},
  { 0x28, tst_28, tst_28_msg, 0},
  { 0x29, tst_29, tst_29_msg, 0},
  { 0x2a, tst_2a, tst_2a_msg, 0},
  { 0x2b, tst_2b, tst_2b_msg, 0},
  { 0x2c, tst_2c, tst_2c_msg, 0},
  { 0x2d, tst_2d, tst_2d_msg, 0},
  { 0x2e, tst_2e, tst_2e_msg, 0},
  { 0x2f, tst_2f, tst_2f_msg, 0},
  { 0x80, tst_80, tst_80_msg, 0},
#ifdef SPARE
  { 0x81, tst_81, tst_81_msg, 0},
  { 0x82, tst_82, tst_82_msg, 0},
  { 0x83, tst_83, tst_83_msg, 0},
  { 0x84, tst_84, tst_84_msg, 0},
#endif
  { 0x85, tst_85, tst_85_msg, 0},
  { 0x86, tst_86, tst_86_msg, 0},
  { 0x87, tst_87, tst_87_msg, 0},
  { 0,} 
};



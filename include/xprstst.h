struct pev_para
{
  int crate;
  int serial;
};

struct xprstst
{
  struct pev_node *pev;
  struct pev_para pev_para;
  struct pev_ioctl_map_pg vme_map_shm;
  struct pev_ioctl_map_pg vme_map_kbuf;
  struct pev_ioctl_map_pg cpu_map_shm;
  struct pev_ioctl_buf    cpu_map_kbuf;
  struct pev_ioctl_map_pg cpu_map_vme_shm;
  struct pev_ioctl_map_pg cpu_map_vme_kbuf;
  struct pev_ioctl_vme_conf vme_conf;
};

struct tst_ctl
{
  int status;
  int test_idx;
  int loop_mode;
  int loop_cnt;
  int log_mode;
  int err_mode;
  char *log_filename;
  FILE *log_file;
  struct xprstst *xt;
  int para_cnt;
  char **para_p;
};

#define SHM_CPU_ADDR( xt)      xt->cpu_map_shm.usr_addr
#define SHM_VME_ADDR( xt)      xt->vme_map_shm.usr_addr
#define SHM_CPU_VME_ADDR( xt)  xt->cpu_map_vme_shm.usr_addr
#define SHM_DMA_ADDR( xt)      (ulong)xt->cpu_map_shm.rem_addr
#define KBUF_CPU_ADDR( xt)     xt->cpu_map_kbuf.u_addr
#define KBUF_VME_ADDR( xt)     xt->vme_map_kbuf.usr_addr
#define KBUF_CPU_VME_ADDR( xt) xt->cpu_map_vme_kbuf.usr_addr
#define KBUF_DMA_ADDR( xt)     (ulong)xt->cpu_map_kbuf.b_addr

#define TST_DMA_NOWAIT 0x00000000
#define TST_DMA_POLL   0x01000000
#define TST_DMA_INTR   0x02000000

#define TST_LOG_OFF    0x00000000
#define TST_LOG_NEW    0x01000000
#define TST_LOG_ADD    0x02000000

#define TST_STS_IDLE      0x00000000
#define TST_STS_STARTED   0x00000001
#define TST_STS_DONE      0x00000002
#define TST_STS_STOPPED   0x00000004
#define TST_STS_ERR       0x00000008

#define TST_ERR_CONT       0x00000000
#define TST_ERR_HALT       0x00000001


char logline[0x101];
#define TST_LOG( x, y) \
sprintf y;\
printf( "%s", logline);fflush( stdout);\
if( x->log_mode){fprintf( x->log_file, "%s", logline);fflush( x->log_file);}

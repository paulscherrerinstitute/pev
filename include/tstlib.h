void *tst_cpu_map_shm( struct pev_ioctl_map_pg *, ulong, uint);
int tst_cpu_unmap_shm( struct pev_ioctl_map_pg *);
void *tst_cpu_map_vme( struct pev_ioctl_map_pg *, ulong, uint, uint);
int tst_cpu_unmap_unmap( struct pev_ioctl_map_pg *);
void *tst_cpu_map_kbuf( struct pev_ioctl_buf *, uint);
int tst_cpu_unmap_kbuf( struct pev_ioctl_buf *);
ulong tst_vme_map_shm( struct pev_ioctl_map_pg *, ulong, uint);
int tst_vme_unmap_shm( struct pev_ioctl_map_pg *);
ulong tst_vme_map_kbuf( struct pev_ioctl_map_pg *, ulong, uint);
int tst_vme_unmap_kbuf( struct pev_ioctl_map_pg *);
uint tst_vme_conf_read( struct pev_ioctl_vme_conf *);
int tst_dma_move_kbuf_shm( ulong, ulong, int, int);
int tst_dma_move_shm_kbuf( ulong, ulong, int, int);
int tst_cpu_cmp( void *, void *, int, int);
int tst_cpu_fill( void *, int, int, int, int);
void *tst_cpu_check( void *, int, int, int, int);

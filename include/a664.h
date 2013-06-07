#ifndef _H_A664
#define _H_A664

typedef unsigned char uchar;
typedef unsigned short ushort;

#define A664_VL_NUM_MAX      0x10000
#define A664_VL_SIZE_MAX        1518                               /* maximum VL sise is 1518 bytes */
#define A664_VL_SIZE_MIN          64                               /* minimum VL size is 64 bytes */
#define A664_VL_BAG_MAX            8                               /* maxinum bag is (2^07)= 128 msec */
#define A664_VL_BAG_MIN            1                               /* minimum bag is (2^0)= 1 msec */
#define A664_BAG_UNIT        1000000                               /* 1 msec = 1000000 nsec */
#define A664_PREAMBLE              8                               /* preamble is 8 bytes */
#define A664_IFG                  12                               /* Inter Frame Gap is 12 byte */
#define A664_EXTRA_SIZE     ( A664_PREAMBLE + A664_IFG)
#define A664_NET_SPEED   ((const unsigned long)(100))            /* A664 nominal speed is 100 Mbit/sec */
#define A664_BIT_PER_BYTE          8
#define A664_TIME_PER_BYTE  ((A664_BIT_PER_BYTE * 1000)/A664_NET_SPEED)  /* at 100 Mbit/sec 1 byte -> 80 nsec */
#define A664_NSEC                 1
#define A664_USEC              1000
#define A664_MSEC           1000000
#define A664_TSLOT_NUM          128
#define A664_VLIST_SIZE_MAX   19048      /* 19048 is the maximum number of VL allowed at 100Mbit, bag=128, size=64 */
#define A664_SCHED_TICK       80*A664_NSEC
#define A664_SCHED_PERIOD    (128*A664_MSEC/A664_SCHED_TICK)
#define A664_SCHED_LAST       0x80000000
#define A664_CAM_SIZE         2048
#define A664_CBM_BASE         0x8000
#define A664_CBM_SIZE         0x8000

#define A664_TX_PORT_TYPE_AFDX        0x0000
#define A664_TX_PORT_TYPE_SAP         0x4000
#define A664_TX_PORT_TYPE_ICMP        0x8000
#define A664_RX_PORT_TYPE_SAMPLING    0x8000
#define A664_RX_PORT_TYPE_QUEUING     0x4000
#define A664_TX_PORT_TYPE_NOFRAG      0x8000

#define A664_SFLASH_CBM_OFF           0x18000
#define A664_CONF_TBL_GA              0x00000

#define A664_OK                     0
#define A664_ERR                   -1
#define A664_TMO                   -2
#define A664_ERR_PORT_INV         -11
#define A664_ERR_PORT_EMPTY       -12
#define A664_ERR_PORT_FULL        -13
#define A664_ERR_CONF_INV         -14
#define A664_ERR_BAD_SIZE         -15
#define A664_ERR_REQ_ERR          -21
#define A664_ERR_REQ_TMO          -22

struct a664_port_status
{
#ifdef BIG_ENDIAN
  ushort size; ushort port_id;       /* 0x0 */
  ushort svl_id; ushort vl_id;       /* 0x4 */
#else
  ushort port_id; ushort size;       /* 0x0 */
  ushort vl_id; ushort svl_id;       /* 0x4 */
#endif
  uint msg_cnt;                      /* 0x8 */
  uint err_cnt;                      /* 0xc */
  uint para[4];
};

struct a664_stats_mac_tx
{
  uint bytes;
  uint frames;
  uint errors;
  uint rsv;
};
struct a664_stats_mac_rx
{
  uint bytes;
  uint frames;
  struct a664_stats_mac_rx_errors
  {
    uint tot;
    uint nodest;
    uint align;
    uint crc;
    uint flen;
    uint internal;
  } errors;
};

struct a664_stats_redundancy
{
  uint first;
  uint errors;
};

struct a664_stats_ip_rx_errors
{
  uint tot;
  uint protos;
  uint checksum;
  uint discards;
};

struct a664_stats_ip_rx_reasm
{
  uint reqds;
  uint ok;
  uint fails;
  uint rsv;
};

struct a664_stats_ip_rx
{
  struct a664_stats_ip_rx_reasm reasm;
  struct a664_stats_ip_rx_errors errors;
};

struct a664_stats_udp_rx
{
  uint errors;
  uint noport;
};

struct a664_stats
{
  struct a664_stats_mac_tx mac_tx_A;
  struct a664_stats_mac_tx mac_tx_B;
  struct a664_stats_mac_rx mac_rx_A;
  struct a664_stats_mac_rx mac_rx_B;
  struct a664_stats_redundancy rm_rx;
  struct a664_stats_ip_rx ip_rx;
  struct a664_stats_udp_rx udp_rx;
};

/* AFDX message header data structure */
struct a664_tx_msg_hdr
{
  uint port_idx;
  uint size;
  uchar ip_des[4];
  ushort rsv; ushort udp_des;
};

struct a664_rx_msg_hdr
{
  uint port_idx;
  uint size;
  uchar ip_des[4];
  ushort rsv; ushort udp_des;
};

struct a664_icmp_hdr
{
  uchar type; uchar code; ushort cks;
  ushort id; ushort seq;
};

/* frame header data structure */
struct a664_frame_hdr
{
  uchar mac_des[6]; /* 0x00 */
  uchar mac_src[6]; /* 0x06 */
  ushort protocol;  /* 0x0c */
  uchar ip_vhl;     /* 0x0e */
  uchar ip_tos;     /* 0x0f */
  ushort ip_len;    /* 0x10 */
  ushort ip_id;     /* 0x12 */
  ushort ip_off;    /* 0x14 */
  uchar ip_ttl;     /* 0x16 */
  uchar ip_prot;    /* 0x17 */
  /*  unsigned short ip_cks;    skipped 0x18 */
  uchar ip_src[4];  /* 0x18 */
  uchar ip_des[4];  /* 0x1c */
  ushort udp_src;   /* 0x20 */
  ushort udp_des;   /* 0x22 */
  ushort udp_len;   /* 0x24 */
  ushort udp_cks;   /* 0x26 */
};                  /* 0x28 */ 
                         /* 0x28 */
/* frame header data structure */
struct a664_rx_frame_desc
{
  uint status;                       /* 0x00 */
  ushort vl_idx; ushort port_idx;    /* 0x04 */
  uint frame_ga;                     /* 0x08 */
  uint time;                         /* 0x0c */
  struct a664_frame_hdr fh;          /* 0x10 */
  uint pad[2];                       /* 0x38 */
};                                   /* 0x40 */

struct a664_tx_port_ctl
{
  struct a664_frame_hdr fh;          /* 0x00 */
  ushort port_id; ushort size;       /* 0x28 */
  ushort vl_id; ushort svl_id;       /* 0x2c */
  uint msg_cnt;                      /* 0x30 */
  uint err_cnt;                      /* 0x34 */
  uint svl_desc_ga;                  /* 0x2c */
  uint rsv   ;                       /* 0x38 */
};                                   /* 0x40 */

struct a664_tx_svl_ctl
{
  uint buf_ga;
  uint buf_size;
  ushort frame_idx; ushort msg_idx;
  ushort frag_offset; ushort msg_id;
};

struct a664_tx_vl_ctl
{
  uint wr_idx[4];                                        /* 0x00 */
  ushort max_size;ushort vl_id;                          /* 0x10 */
  uchar bag; uchar rsv0[3];                              /* 0x14 */
  uchar svl_idx; uchar rsv1; uchar network; uchar sn;    /* 0x18 */
  unsigned short msg_id; unsigned short rsv2;            /* 0x1c */
  uint rd_idx[4];                                        /* 0x20 */
  struct a664_tx_svl_ctl svl_ctl[4];                     /* 0x30 */
  uint rsv[4];                                           /* 0x70 */
};                                                       /* 0x80 */

struct a664_sched_seq
{
  uint tts; 
  uint vl_idx;
};

struct a664_rx_port_ctl
{
  ushort port_id; ushort size;       /* 0x00 */
  uint vl_id;                        /* 0x04 */
  uint msg_cnt;                      /* 0x08 */
  uint err_cnt;                      /* 0x0c */
  uint wr_idx;                       /* 0x10 */
  uint raq_ctl;                      /* 0x14 */
  uint rd_idx;                       /* 0x18 */
  uint buf_ga;                       /* 0x1c */
  uchar ip_des[4];                   /* 0x20 */
  uint udp_des;                      /* 0x24 */
  uint vl_idx;                       /* 0x28 */
  uint dummy[5];                     /* 0x2c */
};                                   /* 0x40 */

struct a664_rx_raq_ctl
{
  uint ip_len;
  uint time;
  ushort frag_offset; ushort msg_size;
  ushort port_idx; ushort rsv;
};

struct a664_rx_icmp_ctl
{
  uint desc_addr;
  uchar ip_des[4];
  uint p2;
  uint p3;
};

struct a664_rx_vl_ctl
{
  uchar snB; uchar rsv0; uchar snA; uchar ic ;           /* 0x00 */
  ushort skew; uchar sn; uchar rm;                       /* 0x04 */
  uint time;                                             /* 0x08 */
  uchar raq_idx; uchar rsv3; ushort vl_id;               /* 0x0c */
  uint ic_errors_A;
  uint ic_errors_B;
  uint ip_forward_A;
  uint ip_forward_B;
  struct a664_rx_raq_ctl raq_ctl[5];                     /* 0x20 */
  struct a664_rx_icmp_ctl icmp_ctl;                      /* 0x70 */
};                                                       /* 0x80 */

struct a664_hc_req_desc
{
  uint cmd;
};

struct a664_map
{
  uint sched_tbl_ga;          /* 0x00 */
  uint tx_vl_tbl_ga;          /* 0x04 */
  uint tx_port_tbl_ga;        /* 0x08 */
  uint sched_period;          /* 0x0c */
  uint rx_vl_tbl_ga;          /* 0x10 */
  uint rx_port_tbl_ga;        /* 0x14 */
  uint rx_icmp_tbl_ga;        /* 0x18 */
  uint hc_req_tbl_ga;         /* 0x1c */
  uint tx_port_num;           /* 0x20 */
  uint rx_port_num;           /* 0x24 */
};

struct a664_conf_tbl_hdr
{
  uint magic;
  uint cks;
  uint len;
  uint conf_tbl_ga;
  char AE_des[32];            /* AFDX Equipment Designation       */
  char AE_pn[32];             /* AFDX Equipment Part Number       */
  char AE_sn[32];             /* AFDX Equipment Serial Number     */
  char AE_lpn[32];            /* AFDX Equipment Load Part Number  */
  char AE_hwv[8];             /* AFDX Equipment Hardware Revision */
  char AE_swv[8];             /* AFDX Equipment Software Revision */
  char AE_tblv[8];            /* AFDX Equipment Table Version     */
  char AE_loc[12];            /* AFDX Equipment Location          */
  ushort mode; ushort ES_id;
  uint net_id;                /* Network identifier               */
  uchar snmp_ip_addr[4];      /* IP address for SNMP trap         */
  uint tx_sched_tbl_ga;       /* base address of scheduling table */
  uint tx_vl_tbl_ga;          /* base address of Tx VL table      */
  uint tx_port_tbl_ga;        /* base address of Tx port table    */
  uint tx_sched_period;
  uint rx_vl_tbl_ga;          /* base address of Rx VL table      */
  uint rx_port_tbl_ga;        /* base address of Rx port table    */
  uint icmp_tbl_ga;           /* base address of ICMP table       */
  uint snmp_wr_idx_ga;            /* address of write index used for SNMP trap */
  ushort tx_port_num; ushort tx_vl_num;
  ushort rx_port_num; ushort rx_vl_num;
  uint snmp_port_index;       /* port index used for SNMP trap     */
  uint snmp_svl_buf_ga;      /* address of SVL queue used for SNMP trap */
  uint hc_req_tbl_ga;         /* base address of request table */
  uint rsv3[3];
};
                                 /* 0x40 */
struct a664_cam_entry
{
  uchar ip1; uchar ip0; ushort vl_id;
  ushort udp; uchar ip3; uchar ip2;
  uint vl_idx;
};

#endif /*  _H_A664 */

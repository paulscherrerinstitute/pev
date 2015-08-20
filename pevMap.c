#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/mman.h>

#include <pevxulib.h>
#define MAP_SPACE_MASK     0xf000
#define MAP_VME_SPACE_MASK 0x0f00

#include <errlog.h>
#include <devLib.h>
#include <iocsh.h>
#include <epicsMutex.h>
#include <epicsExit.h>
#include <epicsTypes.h>
#include <iocsh.h>
#include <epicsExport.h>

#include "pev.h"
#include "pevPrivate.h"

int pevMapDebug = 0;
epicsExportAddress(int, pevMapDebug);

LOCAL struct pevMapEntry {
    struct pevMapEntry* next;
    struct pev_ioctl_map_pg map;
    int refcount;
} *pevMapList[MAX_PEV_CARDS];

LOCAL epicsMutexId pevMapListLock[MAX_PEV_CARDS];

volatile void* pevMapExt(unsigned int card, unsigned int sg_id, unsigned int map_mode, size_t logicalAddress, size_t size, unsigned int flag, size_t localAddress)
{
    struct pevMapEntry** pmapEntry;
    
    if (pevMapDebug)
        printf("pevMap(card=%d, sg_id=0x%02x=%s, map_mode=0x%02x=%s%s%s%s, logicalAddress=0x%zx, size=0x%zx, flags=0x%x, localaddress=0x%zx)\n",
            card, sg_id, pevSgName(sg_id),
            map_mode, pevMapName(map_mode), map_mode & MAP_ENABLE ? "+ENABLE" :"", map_mode & MAP_ENABLE_WR? "+WRITABLE" : "", map_mode & MAP_SWAP_AUTO ? "+SWAP" : "",
            logicalAddress, size, flag, localAddress);

    if (!pevMapListLock[card])
    {
        errlogPrintf("pevMap(card=%d, ...): pevMapListLock is not initialized\n",
            card);
        return NULL;
    }
    epicsMutexLock(pevMapListLock[card]);

    if (!pevMapList[card] && !pevx_init(card))
    {
        errlogPrintf("pevMap(card=%d, ...): pevx_init() failed\n",
            card);
        epicsMutexUnlock(pevMapListLock[card]);
        return NULL;
    }
    
    if (!pevMapList[card])
    {
        int mode = pevx_csr_rd(card, 0x80000404);
        mode |= 1<<5; /* set supervisory mode */
        pevx_csr_wr(card, 0x80000404, mode);
    }

    /* re-use existing maps as far as possible */
    for (pmapEntry = &pevMapList[card]; *pmapEntry; pmapEntry = &(*pmapEntry)->next)
    {
        if ((*pmapEntry)->map.mode != map_mode) continue;
        if ((*pmapEntry)->map.sg_id != sg_id) continue;
        if ((*pmapEntry)->map.flag != flag) continue;
        if ((flag & MAP_FLAG_FORCE) && (*pmapEntry)->map.loc_addr != localAddress) continue;

        /* block has win_size bytes mapped from start address rem_base (in device address space) */
        if ((*pmapEntry)->map.rem_base <= logicalAddress &&
            (*pmapEntry)->map.rem_base + (*pmapEntry)->map.win_size >= logicalAddress + size)
        {
            if (pevMapDebug)
                printf("pevMap(): re-use already allocated map\n");
            break;
        }
    }

    if (!*pmapEntry)
    {
        struct pev_ioctl_map_pg *pmap;

        if (pevMapDebug)
            printf("pevMap(): allocate new map\n");
        *pmapEntry = (struct pevMapEntry*) calloc(1, sizeof(struct pevMapEntry));
        if (*pmapEntry == NULL)
        {
            errlogPrintf("pevMap(): out of memory for new map structure\n");
            epicsMutexUnlock(pevMapListLock[card]);
            return NULL;
        }
        
        pmap=&(*pmapEntry)->map;
        pmap->sg_id = sg_id;
        pmap->mode = map_mode;
        pmap->rem_addr = logicalAddress;
        pmap->size = size;
        pmap->loc_addr = localAddress;
        pmap->flag = flag;

        if (sg_id == MAP_SLAVE_VME && (map_mode & 0xf000) == MAP_SPACE_PCIE && logicalAddress == 0)  /* vme slave window to RAM */
        {
            if (pevMapDebug)
                printf("pevMap(card=%d,... ): VME slave to RAM\n", card);
            /* We need to allocate kernel RAM before we can map it. But for that we need the size first. */
            pmap->mode &= ~MAP_ENABLE; /* disable until we have the RAM */
        }

        /* pevx_map_alloc rounds boundaries as necessary, results in rem_base and win_size */
        pevx_map_alloc(card, pmap);
        if (pmap->win_size < size)
        {
            errlogPrintf("pevMap(card=%d, sg_id=0x%02x=%s, map_mode=0x%02x=%s, logicalAddress=0x%zx, size=0x%zx): allocating map failed. Got size=0x%zx\n",
                card, sg_id, pevSgName(sg_id), map_mode, pevMapName(map_mode), logicalAddress, size, pmap->win_size);
            goto fail;
        }

        if (pevMapDebug)
            printf("pevMap(card=%d, sg_id=0x%02x=%s, map_mode=0x%02x=%s, ...): wanted 0x%lx - 0x%lx, got 0x%lx - 0x%lx @0x%lx\n",
                card, sg_id, pevSgName(sg_id), map_mode, pevMapName(map_mode),
                pmap->rem_addr, pmap->rem_addr + pmap->size,
                pmap->rem_base, pmap->rem_base + pmap->win_size,
                pmap->loc_addr);
        
        /* loc_addr may point to an address not suitable for mmap.
         * It points to the address that maps to rem_addr, but we need loc_base for mmap.
         * Also extend size to win_size. That will map the whole (extended) window.
         * Reconstruct the user requested address later from distance rem_base to logicalAddress.
         */
        pmap->loc_addr -= pmap->rem_addr - pmap->rem_base;
        pmap->size = pmap->win_size;

        if (sg_id != MAP_SLAVE_VME)  /* mmap master windows */
        {
            if (pevx_mmap(card, pmap) == MAP_FAILED)
            {
                errlogPrintf("pevMap(card=%d, sg_id=0x%02x=%s, map_mode=0x%02x=%s, logicalAddress=0x%zx, size=0x%zx): mapping to user space failed: %s\n",
                    card, sg_id, pevSgName(sg_id), map_mode, pevMapName(map_mode), logicalAddress, size, strerror(errno));
                goto fail;
            }
        }
        if (sg_id == MAP_SLAVE_VME && (map_mode & 0xf000) == MAP_SPACE_PCIE && logicalAddress == 0)  /* vme slave window to RAM */
        {
            /* allocate RAM for slave window */
            if (pevMapDebug)
                printf("pevMap(card=%d,... ): allocate RAM\n", card);
            
            pmap->usr_addr = pevDmaRealloc(card, NULL, pmap->win_size);
            
            if (pmap->usr_addr == NULL)
            {
                errlogPrintf("pevMap(card=%d, sg_id=0x%02x=%s, map_mode=0x%02x=%s, logicalAddress=0x%zx, size=0x%zx): pevDmaRealloc() failed: %s\n",
                    card, sg_id, pevSgName(sg_id), map_mode, pevMapName(map_mode), logicalAddress, size, strerror(errno));
                goto fail;
            }
            if (pevMapDebug)
                printf("pevMap(card=%d, sg_id=0x%02x=%s, map_mode=0x%02x=%s, logicalAddress=0x%zx, size=0x%zx): buffer=%p\n",
                    card, sg_id, pevSgName(sg_id), map_mode, pevMapName(map_mode), logicalAddress, size, pmap->usr_addr);  

            /* now re-map to RAM buffer */
            pmap->mode = map_mode;
            pmap->rem_addr = pevDmaUsrToBusAddr(card, pmap->usr_addr);
            pevx_map_modify(card, pmap);
        }
    }

    (*pmapEntry)->refcount++;
    epicsMutexUnlock(pevMapListLock[card]);
    if (pevMapDebug)
        printf("pevMap(card=%d, sg_id=0x%02x=%s, map_mode=0x%02x=%s, logicalAddress=0x%zx, size=0x%zx): rem_base=0x%lx, usr_addr=%p\n",
            card, sg_id, pevSgName(sg_id), map_mode, pevMapName(map_mode), logicalAddress, size, (*pmapEntry)->map.rem_base, (*pmapEntry)->map.usr_addr);

    /* usr_addr mapps to rem_base, user logicalAddress needs offset into this block */
    return logicalAddress - (*pmapEntry)->map.rem_base + (char*)(*pmapEntry)->map.usr_addr;
fail:
    if ((*pmapEntry)->map.win_size != 0)
        pevx_map_free(card, &(*pmapEntry)->map);
    free(*pmapEntry);
    *pmapEntry = NULL;
    epicsMutexUnlock(pevMapListLock[card]);
    return NULL;
}

void pevUnmap(volatile void* ptr)
{
    struct pevMapEntry** pmapEntry;
    struct pevMapEntry* mapEntry;
    unsigned int card;

    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        epicsMutexLock(pevMapListLock[card]);
        for (pmapEntry = &pevMapList[card]; *pmapEntry; pmapEntry = &(*pmapEntry)->next)
        {
            mapEntry = *pmapEntry;
            if (mapEntry->map.usr_addr && ptr >= mapEntry->map.usr_addr && ptr < mapEntry->map.usr_addr + mapEntry->map.win_size)
            {
                if (--mapEntry->refcount == 0)
                {
                    if (pevMapDebug)
                        printf("pevUnmap(): releasing memory map card %d %s base=0x%08lx size=0x%x=%uMB\n",
                            card, pevMapName(mapEntry->map.mode),
                            mapEntry->map.rem_base, mapEntry->map.win_size, mapEntry->map.win_size>>20);
                    if (mapEntry->map.sg_id == MAP_SLAVE_VME)
                        pevDmaRealloc(card, mapEntry->map.usr_addr, 0);
                    else
                        pevx_munmap(card, &mapEntry->map);
                    pevx_map_free(card, &mapEntry->map);
                    *pmapEntry = mapEntry->next;
                    free (mapEntry);
                    epicsMutexUnlock(pevMapListLock[card]);
                    return;
                }
            }
        }
        epicsMutexUnlock(pevMapListLock[card]);
    }
    errlogPrintf("pevUnmap(%p): pointer does not belong to any map\n", ptr);
}

const char* pevSgName(unsigned int sg_id)
{
    switch (sg_id)
    {
        case MAP_PCIE_MEM:  return "MEM";
        case MAP_PCIE_PMEM: return "PMEM";
        case MAP_PCIE_CSR:  return "PCIe CSR";
        case MAP_VME_SLAVE: return "VME A32";
        case MAP_VME_ELB:   return "VME ELB";
        default:            return "????????";
    }
}

const char* pevMapName(unsigned int map_mode)
{
    switch (map_mode & MAP_SPACE_MASK)
    {
        case MAP_SPACE_VME:
        {
            switch (map_mode & MAP_VME_SPACE_MASK)
            {
                case MAP_VME_CR:   return "VME CSR";
                case MAP_VME_A16:  return "VME A16";
                case MAP_VME_A24:  return "VME A24";
                case MAP_VME_A32:  return "VME A32";
                case MAP_VME_BLT:  return "VME BLT";
                case MAP_VME_MBLT: return "VME MBLT";
                default:           return "VME ???";
            }
        }
        case MAP_SPACE_PCIE:       return "PCIe";
        case MAP_SPACE_SHM:        return "SHM";
        case MAP_SPACE_USR:        return "USR";
        case MAP_SPACE_USR1:       return "USR1";
        case MAP_SPACE_USR2:       return "USR2";
        default:                   return "???";
    }
}

size_t pevMapPageSize(unsigned int card, unsigned int sg_id)
{
    struct pev_ioctl_map_ctl map_ctl;

    map_ctl.sg_id = sg_id;
    map_ctl.map_p = NULL;
    map_ctl.pg_size = 0;
    pevx_map_read(card, &map_ctl);
    return map_ctl.pg_size;
}

int pevMapGetMapInfo(const volatile void* address, struct pevMapInfo* info)
{
    struct pevMapEntry* mapEntry;
    unsigned int card;

    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        for (mapEntry = pevMapList[card]; mapEntry; mapEntry = mapEntry->next)
        {
            if (address >= mapEntry->map.usr_addr &&
                address <= mapEntry->map.usr_addr + mapEntry->map.win_size)
            {
                info->name  = pevMapName(mapEntry->map.mode);
                info->card  = card;
                info->base  = mapEntry->map.usr_addr - mapEntry->map.rem_base;
                info->size  = mapEntry->map.win_size;
                return 1;
            }
        }
    }
    return 0;
}

void pevMapShow(int vmeOnly)
{
    struct pevMapEntry* mapEntry;
    unsigned int card;
    unsigned int index;
    int anything_reported = 0;
    
    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        struct pev_ioctl_vme_conf vme_conf;
        pevx_init(card);
        if (pevx_vme_conf_read(card, &vme_conf) != 0) continue;
        if (pevMapList[card] || (pevx_vme_conf_read(card, &vme_conf) == 0 && (vme_conf.slv_ena & VME_SLV_ENA)))
        {
            int supervisory = pevx_csr_rd(card, 0x80000404) & (1<<5);
            if (vme_conf.slv_ena & VME_SLV_ENA)
            {
                printf("      %-8s 0x%08x -> %-8s               size=0x%08x=%uMB\n",
                    "VME A32", vme_conf.a32_base, "VME SLAVE",
                    vme_conf.a32_size, vme_conf.a32_size>>20);
            }
            for (mapEntry = pevMapList[card], index = 1; mapEntry; mapEntry = mapEntry->next, index++)
            {
                struct pev_ioctl_map_pg *pmap = &mapEntry->map;
                struct pev_ioctl_map_ctl map_ctl;
                int space = pmap->mode & MAP_SPACE_MASK;
                
                if (vmeOnly && space != MAP_SPACE_VME && pmap->sg_id != MAP_VME_SLAVE) continue;

                map_ctl.sg_id = pmap->sg_id;
                map_ctl.map_p = NULL;
                map_ctl.pg_size = 0;
                pevx_map_read(card, &map_ctl);

                printf("   %d: %-8s 0x%08lx -> %-8s%3s 0x%08lx  size=0x%08x=%uMB %s %s\n",
                    index,
                    pevSgName(pmap->sg_id),
                    pmap->sg_id == MAP_PCIE_MEM || pmap->sg_id == MAP_PCIE_PMEM ? (size_t)pmap->usr_addr :
                    pmap->loc_addr + (pmap->sg_id == MAP_SLAVE_VME ? vme_conf.a32_base : 0),
                    space == MAP_SPACE_PCIE && pmap->usr_addr ? "RAM" : pevMapName(pmap->mode),
                    space == MAP_SPACE_VME ? supervisory ? "SUP" : "USR" : "",
                    space == MAP_SPACE_PCIE && pmap->usr_addr ? (size_t)pmap->usr_addr : pmap->rem_base,
                    pmap->win_size, pmap->win_size>>20,
                    (pmap->mode & MAP_ENABLE) ? (pmap->mode & MAP_ENABLE_WR) ? "R/W" : "R" : "disabled",
                    (pmap->mode & MAP_SWAP_AUTO) ? "auto-swap" : "");
            }
            anything_reported = 1;
        }
    }
    if (!anything_reported)
    {
        printf("  No map installed\n");
    }
}

static const iocshFuncDef pevMapShowDef =
    { "pevMapShow", 0, NULL };

static void pevMapShowFunc (const iocshArgBuf *args)
{
    pevMapShow(0);
}

static const iocshFuncDef vmeMapShowDef =
    { "vmeMapShow", 0, NULL };

static void vmeMapShowFunc (const iocshArgBuf *args)
{
    pevMapShow(1);
}

void pevMapDisplay(unsigned int card, unsigned int map, size_t start, unsigned int dlen, size_t bytes)
{
    struct pevMapEntry* mapEntry;
    static size_t offset = 0;
    static size_t save_bytes = 128;
    static unsigned int save_dlen = 2;
    static unsigned int save_card = 0;
    static unsigned int save_map = 0;
    unsigned int index;
    unsigned int size;
    char buffer[16];

    unsigned int i, j;
    
    if (card > MAX_PEV_CARDS || !pevMapList[card])
    {
        printf("invalid card number %d\n", card);
        return;
    }
    index = 1;
    for (mapEntry = pevMapList[card]; mapEntry; mapEntry = mapEntry->next)
    {
        if (index++ == map) break;
    }
    if (!mapEntry)
    {
        printf("invalid map number %d\n", map);
        return;
    }
    if (card != save_card || map != save_map)
    {
        save_card = card;
        save_map = map;
        offset = 0;
    }
    if (start > 0 || dlen || bytes) offset = start;
    if (bytes) save_bytes = bytes; else bytes = save_bytes;
    switch (dlen)
    {
        case 0: 
            dlen = save_dlen;
            break;
            /* round down size to dlen precision */
        case 4:
            bytes &= ~3;
        case 2:
            bytes &= ~1;
        case 1:
            save_dlen = dlen;
            break;
        default:
            errlogPrintf("illegal dlen %d, must be 1, 2, or 4\n", dlen);
            return;
    }
    
    /* do not read over the end of the map window */
    size = mapEntry->map.win_size;
    if (offset >= size)
    {
        errlogPrintf("address 0x%x out of range\n", offset);
        return;
    }
    if (offset + bytes > size) bytes = size - offset;
    
    printf ("%s base=0x%08lx @%p\n", pevMapName(mapEntry->map.mode), mapEntry->map.rem_base, mapEntry->map.usr_addr);
    if (mapEntry->map.usr_addr == NULL)
    {
        printf ("not mapped to user space\n");
        return;
    }
    for (i = 0; i < bytes; i += 16)
    {
        printf ("%08x: ", offset);
        switch (dlen)
        {
            case 1:
                for (j = 0; j < 16; j++)
                {
                    epicsUInt8 x;
                    if (i + j >= bytes)
                    {
                        printf ("   ");
                    }
                    else
                    {
                        x = *(epicsUInt8*)((char*)mapEntry->map.usr_addr + offset + j);
                        *(epicsUInt8*)(buffer + j) = x;
                        printf ("%02x ", x);
                    }
                }
                break;
            case 2:
                for (j = 0; j < 16; j+=2)
                {
                    epicsUInt16 x;
                    if (i + j >= bytes)
                    {
                        printf ("     ");
                    }
                    else
                    {
                        x = *(epicsUInt16*)((char*)mapEntry->map.usr_addr + offset + j);
                        *(epicsUInt16*)(buffer + j) = x;
                        printf ("%04x ", x);
                    }
                }
                break;
            case 4:
                for (j = 0; j < 16; j+=4)
                {
                    epicsUInt32 x;
                    if (i + j >= bytes)
                    {
                        printf ("       ");
                    }
                    else
                    {
                        x = *(epicsUInt32*)((char*)mapEntry->map.usr_addr + offset + j);
                        *(epicsUInt32*)(buffer + j) = x;
                        printf ("%08x ", x);
                    }
                }
        }
        for (j = 0; j < 16; j++)
        {
            if (offset + j >= size) break;
            printf ("%c", isprint((unsigned char)buffer[j]) ? buffer[j] : '.');
        }
        offset += 16;
        printf ("\n");
    }
}

static const iocshArg pevMapDisplayArg0 = { "card", iocshArgInt };
static const iocshArg pevMapDisplayArg1 = { "map", iocshArgInt };
static const iocshArg pevMapDisplayArg2 = { "start", iocshArgInt };
static const iocshArg pevMapDisplayArg3 = { "dlen", iocshArgInt };
static const iocshArg pevMapDisplayArg4 = { "bytes", iocshArgInt };
static const iocshArg * const pevMapDisplayArgs[] = {
    &pevMapDisplayArg0,
    &pevMapDisplayArg1,
    &pevMapDisplayArg2,
    &pevMapDisplayArg3,
    &pevMapDisplayArg4,
};

static const iocshFuncDef pevMapDisplayDef =
    { "pevMapDisplay", 5, pevMapDisplayArgs };
    
static void pevMapDisplayFunc (const iocshArgBuf *args)
{
    pevMapDisplay(
        args[0].ival, args[1].ival, args[2].ival, args[3].ival, args[4].ival);
}

void pevMapPut(unsigned int card, unsigned int map, size_t offset, unsigned int dlen, int value)
{
    struct pevMapEntry* mapEntry;
    unsigned int index;

    if (card > MAX_PEV_CARDS || !pevMapList[card])
    {
        printf("invalid card number %d\n", card);
        return;
    }
    index = 1;
    for (mapEntry = pevMapList[card]; mapEntry; mapEntry = mapEntry->next)
    {
        if (index++ == map) break;
    }
    if (!mapEntry)
    {
        printf("invalid map number %d\n", map);
        return;
    }
    if (offset+dlen >= mapEntry->map.win_size)
    {
        errlogPrintf("address 0x%x out of range\n", offset);
        return;
    }
    if (mapEntry->map.usr_addr == NULL)
    {
        printf ("not mapped to user space\n");
        return;
    }
    switch (dlen)
    {
        case 1:
            *(epicsUInt8*)((char*)mapEntry->map.usr_addr + offset) = (epicsUInt8)value;
            break;
        case 2:
            *(epicsUInt16*)((char*)mapEntry->map.usr_addr + offset) = (epicsUInt16)value;
            break;
        case 4:
            *(epicsUInt32*)((char*)mapEntry->map.usr_addr + offset) = (epicsUInt32)value;
            break;
        default:
            errlogPrintf("illegal dlen %d, must be 1, 2, or 4\n", dlen);
            return;
    }
}

static const iocshArg pevMapPutArg0 = { "card", iocshArgInt };
static const iocshArg pevMapPutArg1 = { "map", iocshArgInt };
static const iocshArg pevMapPutArg2 = { "offset", iocshArgInt };
static const iocshArg pevMapPutArg3 = { "dlen", iocshArgInt };
static const iocshArg pevMapPutArg4 = { "value", iocshArgInt };
static const iocshArg * const pevMapPutArgs[] = {
    &pevMapPutArg0,
    &pevMapPutArg1,
    &pevMapPutArg2,
    &pevMapPutArg3,
    &pevMapPutArg4,
};

static const iocshFuncDef pevMapPutDef =
    { "pevMapPut", 5, pevMapPutArgs };
    
static void pevMapPutFunc (const iocshArgBuf *args)
{
    pevMapPut(
        args[0].ival, args[1].ival, args[2].ival, args[3].ival, args[4].ival);
}

LOCAL void pevMapExit(void* dummy)
{
    struct pevMapEntry* mapEntry;
    unsigned int card;

    /* release memory maps */
    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        for (mapEntry = pevMapList[card]; mapEntry; mapEntry = mapEntry->next)
        {
            if (pevMapDebug)
                printf("pevMapExit(): releasing memory map card %d %s base=0x%08lx size=0x%x=%uMB\n",
                    card,
                    pevMapName(mapEntry->map.mode),
                    mapEntry->map.rem_base,mapEntry->map.win_size, mapEntry->map.win_size>>20);
            if (mapEntry->map.usr_addr)
            {
                if (mapEntry->map.sg_id == MAP_SLAVE_VME)
                    pevDmaRealloc(card, mapEntry->map.usr_addr, 0);
                else
                    pevx_munmap(card, &mapEntry->map);
            }
            pevx_map_free(card, &mapEntry->map);
        }
    }
    if (pevMapDebug)
        printf("pevMapExit(): done\n");
}

int pevMapInit(void)
{
    unsigned int card;

    pevInstallMapInfo(pevMapGetMapInfo);

    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        pevMapListLock[card] = epicsMutexCreate();
        if (!pevMapListLock[card])
        {
            errlogPrintf("pevMapInit(): epicsMutexCreate() failed\n");
            return S_dev_noMemory;
        }
    }

    epicsAtExit(pevMapExit, NULL);
    
    iocshRegister(&pevMapShowDef, pevMapShowFunc);
    iocshRegister(&vmeMapShowDef, vmeMapShowFunc);
    iocshRegister(&pevMapDisplayDef, pevMapDisplayFunc);
    iocshRegister(&pevMapPutDef, pevMapPutFunc);
    
    return S_dev_success;
}


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/mman.h>

#include <pevxulib.h>

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

volatile void* pevMap(unsigned int card, unsigned int sg_id, unsigned int map_mode, size_t logicalAddress, size_t size)
{
    struct pevMapEntry** pmapEntry;
    
    if (pevMapDebug)
        printf("pevMap(card=%d, sg_id=0x%02x, mode=0x%02x, logicalAddress=0x%zx, size=0x%zx)\n",
            card, sg_id, map_mode, logicalAddress, size);

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
        if (pevMapDebug)
            printf("pevMap(): allocate new map\n");
        *pmapEntry = (struct pevMapEntry*) calloc(1, sizeof(struct pevMapEntry));
        if (*pmapEntry == NULL)
        {
            errlogPrintf("pevMap: out of memory for new map structure\n");
            epicsMutexUnlock(pevMapListLock[card]);
            return NULL;
        }
        (*pmapEntry)->map.sg_id = sg_id;
        (*pmapEntry)->map.mode = map_mode;
        (*pmapEntry)->map.rem_addr = logicalAddress;                                
        (*pmapEntry)->map.size = size;                                              

        /* pevx_map_alloc rounds boundaries as necessary, results in rem_base and win_size */
        pevx_map_alloc(card, &(*pmapEntry)->map);
        if ((*pmapEntry)->map.win_size < (*pmapEntry)->map.size)
        {
            errlogPrintf("pevMap(card=%d, sg_id=0x%02x, mode=0x%02x, logicalAddress=0x%zx, size=0x%zx): allocating map failed\n",
                card, sg_id, map_mode, logicalAddress, size);
            free(*pmapEntry);
            *pmapEntry = NULL;
            epicsMutexUnlock(pevMapListLock[card]);
            return NULL;
        }

        if (pevMapDebug)
            printf("pevMap(card=%d, sg_id=0x%02x, mode=0x%02x, ...): wanted 0x%lx - 0x%lx, got 0x%lx - 0x%lx @0x%lx\n",
                card, (*pmapEntry)->map.sg_id, (*pmapEntry)->map.mode,
                (*pmapEntry)->map.rem_addr, (*pmapEntry)->map.rem_addr + (*pmapEntry)->map.size,
                (*pmapEntry)->map.rem_base, (*pmapEntry)->map.rem_base + (*pmapEntry)->map.win_size,
                (*pmapEntry)->map.loc_addr);
        
        /* loc_addr may point to an address not suitable for mmap.
         * It points to the address that maps to rem_addr, but we need rem_base for mmap.
         * Also extend size to win_size. That will map the whole (extended) window.
         * Reconstruct the user requested address later from distance rem_base to logicalAddress.
         */
        (*pmapEntry)->map.loc_addr -= (*pmapEntry)->map.rem_addr - (*pmapEntry)->map.rem_base;
        (*pmapEntry)->map.size = (*pmapEntry)->map.win_size;

        if (pevx_mmap(card, &(*pmapEntry)->map) == MAP_FAILED)
        {
            errlogPrintf("pevMap(card=%d, sg_id=0x%02x, mode=0x%02x, logicalAddress=0x%zx, size=0x%zx): mapping to user space failed: %s\n",
                card, sg_id, map_mode, logicalAddress, size, strerror(errno));
            pevx_map_free(card, &(*pmapEntry)->map);
            free(*pmapEntry);
            *pmapEntry = NULL;
            epicsMutexUnlock(pevMapListLock[card]);
            return NULL;
        }
    }

    (*pmapEntry)->refcount++;
    epicsMutexUnlock(pevMapListLock[card]);
    if (pevMapDebug)
        printf("pevMap(card=%d, sg_id=0x%02x, mode=0x%02x, logicalAddress=0x%zx, size=0x%zx): rem_base=0x%lx, usr_addr=%p\n",
            card, sg_id, map_mode, logicalAddress, size, (*pmapEntry)->map.rem_base, (*pmapEntry)->map.usr_addr);

    /* usr_addr mapps to rem_base, user logicalAddress needs offset into this block */
    return logicalAddress - (*pmapEntry)->map.rem_base + (char*)(*pmapEntry)->map.usr_addr;
}

void pevUnmap(void* ptr)
{
    struct pevMapEntry* mapEntry;
    unsigned int card;

    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        epicsMutexLock(pevMapListLock[card]);
        for (mapEntry = pevMapList[card]; mapEntry; mapEntry = mapEntry->next)
        {
            if (mapEntry->refcount == 0 || mapEntry->map.usr_addr == MAP_FAILED || mapEntry->map.usr_addr == NULL) continue;
            if (ptr >= mapEntry->map.usr_addr && ptr < mapEntry->map.usr_addr + mapEntry->map.win_size)
            {
                if (--mapEntry->refcount == 0)
                {
                    if (pevMapDebug)
                        printf("pevUnmap(): releasing memory map card %d %s base=0x%08lx size=0x%x\n",
                            card,
                            pevMapName(mapEntry->map.mode),
                            mapEntry->map.rem_base,mapEntry->map.win_size);
                    pevx_map_free(card, &mapEntry->map);
                    epicsMutexUnlock(pevMapListLock[card]);
                    return;
                }
            }
        }
        epicsMutexUnlock(pevMapListLock[card]);
    }
}

const char* pevSgName(unsigned int sg_id)
{
    switch (sg_id)
    {
        case MAP_PCIE_MEM:  return "PCIe MEM";
        case MAP_PCIE_PMEM: return "PCIe PMEM";
        case MAP_PCIE_CSR:  return "PCIe CSR";
        case MAP_VME_SLAVE: return "VME SLAVE";
        case MAP_VME_ELB:   return "VME ELB";
        default:            return "????????";
    }
}

const char* pevMapName(unsigned int map_mode)
{
    switch (map_mode & 0xF000)
    {
        case MAP_SPACE_VME:
        {
            switch (map_mode & 0x0f00)
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

void pevMapShow(const iocshArgBuf *args)
{
    struct pevMapEntry* mapEntry;
    unsigned int card;
    unsigned int index;
    
    printf("pev memory maps:\n");
    for (card = 0; card < MAX_PEV_CARDS; card++)
    {
        if (pevMapList[card])
        {
            printf(" card %d: (VME maps use %s mode)\n",
                card, pevx_csr_rd(card, 0x80000404) & (1<<5) ?
                    "supervisory" : "user");
            index = 0;
            for (mapEntry = pevMapList[card]; mapEntry; mapEntry = mapEntry->next)
            {
                printf("  %d: %-9s to %-8s %s swap=%s base=0x%08lx size=0x%08x %3dMB\n",
                    index++,
                    pevSgName(mapEntry->map.sg_id), pevMapName(mapEntry->map.mode), 
                    mapEntry->map.mode & MAP_ENABLE ? mapEntry->map.mode & MAP_ENABLE_WR ? "rdwr" : "read" : "disa",
                    mapEntry->map.mode & MAP_SWAP_AUTO ? "auto" : "off ",
                    mapEntry->map.rem_base,
                    mapEntry->map.win_size,
                    mapEntry->map.win_size>>20);
            }
        }
    }
}

int pevMapGetMapInfo(const void* address, struct pevMapInfo* info)
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
                info->start = mapEntry->map.rem_base;
                info->size  = mapEntry->map.win_size;
                return 1;
            }
        }
    }
    return 0;
}

void pevMapDisplay(unsigned int card, int map, size_t start, unsigned int dlen, size_t bytes)
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

    int i, j;
    
    if (card > MAX_PEV_CARDS || !pevMapList[card])
    {
        printf("invalid card number %d\n", card);
        return;
    }
    index = 0;
    for (mapEntry = pevMapList[card]; mapEntry; mapEntry = mapEntry->next)
    {
        if (index++ == map) break;
    }
    if (!mapEntry)
    {
        printf("invalid index number %d\n", map);
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
    for (i = 0; i < bytes; i += 16)
    {
        printf ("%08x: ", offset + i);
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

static const iocshFuncDef pevMapShowDef =
    { "pevMapShow", 0, NULL };

static const iocshFuncDef vmeMapShowDef =
    { "vmeMapShow", 0, NULL };

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
                printf("pevMapExit(): releasing memory map card %d %s base=0x%08lx size=0x%x\n",
                    card,
                    pevMapName(mapEntry->map.mode),
                    mapEntry->map.rem_base,mapEntry->map.win_size);
            if (mapEntry->map.usr_addr != MAP_FAILED && mapEntry->map.usr_addr != NULL)
                pevx_munmap(card, &mapEntry->map);
            if (mapEntry->map.win_size != 0)
                pevx_map_free(card, &mapEntry->map);
        }
    }
}

int pevMapInit()
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
    
    iocshRegister(&pevMapShowDef, pevMapShow);
    iocshRegister(&vmeMapShowDef, pevMapShow);
    iocshRegister(&pevMapDisplayDef, pevMapDisplayFunc);
    
    return S_dev_success;
}


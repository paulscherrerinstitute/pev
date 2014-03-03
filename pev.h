#ifndef pev_h
#define pev_h

#ifdef __cplusplus
extern "C" {
#endif

int pevInit();

/**** memory maps ****/

/*
    pevMap
    
    Map card memory from a specified address space (sg_id, see pev manual) to program address space.
*/

volatile void* pevMap(unsigned int card, unsigned int sg_id, unsigned int map_mode, size_t logicalAddress, size_t size);
void pevUnmap(void*);

const char* pevSgName(unsigned int sg_id);

const char* pevMapName(unsigned int map_mode);

/**** interrupts *****/

/*
  pevIntrConnect
   
  Install user func as interrupt handler.
    * func will be called in a separate high priority thread
    * func can have up to 3 arguments: void* usr, int src_id, int vec_id
    * func must not block
    * func must not waste stack
    * if src_id is EVT_SRC_VME (0x10) then all 7 VME interrupt lines (0x10 ... 0x17) will be registered
    * if vec_id is 0 then func is called for any value of vec_id
    * func may inspect current setting for vec_id and src_id when called
    
  All interrupt sources start in disabled mode.


  pevIntrDisconnect
  
  Uninstall a previously installed interrupt handler.
  All arguments must be the same as used for pevConnectInterrupt, in order to avoid uninstalling the wrong handler,
  except for usr, wich can be NULL.
*/

int pevIntrConnect(unsigned int card, unsigned int src_id, unsigned int vec_id, void (*func)(), void* usr);

int pevIntrDisconnect(unsigned int card, unsigned int src_id, unsigned int vec_id, void (*func)(), void* usr);

int pevIntrEnable(unsigned int card, unsigned int src_id);

int pevIntrDisable(unsigned int card, unsigned int src_id);

/**** DMA ****/

/*
  DMA to/from user memory requires special DMA aware buffers. Allocate a buffer with pevDmaRealloc.
  (Which can also be used to resize or release the buffer.)
  The return value is a memory page aligned user space virtual address (or NULL on failure).
  When using this buffer for DMA transfer specify DMA_SPACE_BUF as src_space or des_space, respectively.
  Or simply use pevDmaToBuffer/pevDmaFromBuffer.
  These transfers do not need to start at the buffer start address but must completely fit into the buffer.
  The Wait versions block until the DMA transfer is completed (or failed), the non-Wait versions return
  immediately (fail if the request cannot be queued) and call the user provided callback function when
  the DMA is completed (or failed).
*/

#define DMA_SPACE_BUF DMA_SPACE_MASK

void* pevDmaRealloc(unsigned int card, void* oldptr, size_t size);

extern inline void* pevDmaAlloc(unsigned int card, size_t size)
{
    return pevDmaRealloc(card, NULL, size);
}

extern inline void* pevDmaFree(unsigned int card, void* oldptr)
{
    return pevDmaRealloc(card, oldptr, 0);
}

const char* pevDmaSpaceName(unsigned int dma_space);

typedef void (*pevDmaCallback)(void* usr, int status);

int pevDmaTransfer(unsigned int card, unsigned int src_space, size_t src_addr, unsigned int des_space, size_t des_addr, size_t size, unsigned int swap_mode,
    unsigned int priority, pevDmaCallback callback, void *usr);

extern inline int pevDmaTransferWait(unsigned int card, unsigned int src_space, size_t src_addr, unsigned int des_space, size_t des_addr, size_t size, unsigned int swap_mode)
{
    return pevDmaTransfer(card, src_space, src_addr, des_space, des_addr, size, swap_mode, 0, NULL, NULL);
}

extern inline int pevDmaFromBuffer(unsigned int card, const void* buffer, unsigned int des_space, unsigned int des_addr, size_t size, unsigned int swap_mode,
    unsigned int priority, pevDmaCallback callback, void *usr)
{
    return pevDmaTransfer(card, DMA_SPACE_BUF, (size_t)buffer, des_space, des_addr, size, swap_mode, priority, callback, usr);
}

extern inline int pevDmaToBuffer(unsigned int card, unsigned int src_space, size_t src_addr, void* buffer, size_t size, unsigned int swap_mode,
    unsigned int priority, pevDmaCallback callback, void *usr)
{
    return pevDmaTransfer(card, src_space, src_addr, DMA_SPACE_BUF, (size_t)buffer, size, swap_mode, priority, callback, usr);
}

extern inline int pevDmaFromBufferWait(unsigned int card, void* buffer, unsigned int des_space, unsigned int des_addr, size_t size, unsigned int swap_mode)
{
    return pevDmaTransfer(card, DMA_SPACE_BUF, (size_t)buffer, des_space, des_addr, size, swap_mode, 0, NULL, NULL);
}

extern inline int pevDmaToBufferWait(unsigned int card, unsigned int src_space, size_t src_addr, void* buffer, size_t size, unsigned int swap_mode)
{
    return pevDmaTransfer(card, src_space, src_addr, DMA_SPACE_BUF, (size_t)buffer, size, swap_mode, 0, NULL, NULL);
}

void pevDmaReport(int level);

#ifdef __cplusplus
}
#endif

#endif /* pev.h */

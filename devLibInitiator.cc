#include <devLib.h>
#include <stdio.h>

#if defined(pdevLibVirtualOS) && !defined(devLibVirtualOS)
#define devLibVirtualOS devLibVME
#endif

extern devLibVirtualOS pevVirtualOS;

int pevDevLibVMEInit ()
{
	printf(">>>>>>>>>>>>>>>>> in pevDevLibVMEInit()\n");

	devLibVirtualOS *pdevLibVirtualOSpdevLibVME = &pevVirtualOS;
	return 0;
}

static int pevDevLibVMEInitLocal = pevDevLibVMEInit();

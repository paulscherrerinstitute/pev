#include <devLib.h>
#include <stdio.h>

#if defined(pdevLibVirtualOS) && !defined(devLibVirtualOS)
#define devLibVirtualOS devLibVME
#endif

extern devLibVirtualOS pevVirtualOS;

int pevDevLibVMEInit ()
{
	pdevLibVirtualOS = &pevVirtualOS;
	return 0;
}

static int pevDevLibVMEInitLocal = pevDevLibVMEInit();

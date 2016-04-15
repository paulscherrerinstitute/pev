extern int pevDebug;

#define MAX_PEV_CARDS 16

struct pevMapInfo {
    const char* name;
    unsigned int card;
    const volatile void* base;
    size_t addr;
    size_t size;
};

int pevGetMapInfo(const volatile void* address, struct pevMapInfo* info);
int pevInstallMapInfo(int (*)(const volatile void* address, struct pevMapInfo* info));
int pevMapInit(void);
int pevIntrInit(void);
int pevDmaInit(void);
int pevInitCard(int card);

size_t pevDmaUsrToBusAddr(unsigned int card, void* useraddr);

void pevVmeShow(void);

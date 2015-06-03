extern int pevDebug;

#define MAX_PEV_CARDS 16

struct pevMapInfo {
    const char* name;
    unsigned int card;
    size_t start;
    size_t size;
};

int pevGetMapInfo(const void* address, struct pevMapInfo* info);
int pevInstallMapInfo(int (*)(const void* address, struct pevMapInfo* info));

int pevMapInit(void);
int pevIntrInit(void);
int pevDmaInit(void);

size_t pevDmaUsrToBusAddr(unsigned int card, void* useraddr);

void pevVmeShow(void);

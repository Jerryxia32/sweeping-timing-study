#include<inttypes.h>
#include"gadget.h"

void
sweep_line(uint64_t* memPool, uint64_t momData) {
  for(int i=0; i<PTRS_IN_CACHELINE; i++) {
    if(x86_testsubset(momData, memPool[i])) {
      memPool[i] = NORMAL_DATA;
    }
  }
}

void
trash_cache(volatile uint64_t* cacheTrashPool) {
  for(ssize_t i=0; i<CACHE_TRASH_SIZE/sizeof(uint64_t); i++) {
    uint64_t haha = cacheTrashPool[i];
  }
}

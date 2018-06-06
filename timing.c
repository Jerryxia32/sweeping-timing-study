#include<sys/stat.h>
#include<stdio.h>
#include<stdlib.h>
#include<inttypes.h>
#include<assert.h>
#include<errno.h>
#include<string.h>
#include"gadget.h"

int
main(int argc, char* argv[]) {
  if(argc != 3) {
    fprintf(stderr, "Need two arguments, the filename of the vecbin and the "
        "number of iterations over which to take the average.\n");
    exit(-1);
  }

  char* vecbinName = argv[1];
  // FIXME: no checking here to see if the number is valid.
  int ITER = atoi(argv[2]);

  struct stat vecbinSt;
  size_t fileSize, realSize, ptrNum, cachelineNum, pageNum;
  if(stat(vecbinName, &vecbinSt) == 0) {
    fileSize = vecbinSt.st_size;
    // This is the size of the original core dump.
    realSize = fileSize * 8 * PTR_SIZE;
    // This is how many pointers, cachelines, pages are there in the core dump.
    ptrNum = realSize / PTR_SIZE;
    cachelineNum = realSize / CACHELINE_SIZE;
    pageNum = realSize / PAGE_SIZE;
  } else {
    fprintf(stderr, "Cannot determine size of %s: %s\n",
        vecbinName, strerror(errno));
    exit(-1);
  }

  // Read the file into a big chunk of memory.
  FILE* vecbinFile = fopen(vecbinName, "r");
  char* vecbinPool = (char*)calloc(fileSize, 1);
  fread(vecbinPool, fileSize, 1, vecbinFile);

  // Read from this array to completely trash the cache.
  volatile uint64_t* cacheTrashPool =
      (volatile uint64_t*)calloc(CACHE_TRASH_SIZE, 1);

  // Create pools, each pool indicates whether this cacheline or page is dirty
  // with capabilities.
  char* cachelinePool = (char*)calloc(cachelineNum/8, 1);
  char* pagePool = (char*)calloc(pageNum/8, 1);
  // Simulate the actual memory pool with actual pointers.
  uint64_t* memPool = (uint64_t*)calloc(realSize, 1);

  // Populate the pools.
  for(ssize_t i=0; i<ptrNum; i++) {
    if(bitarray_read(vecbinPool, i)) {
      // This index has a pointer.
      bitarray_write(cachelinePool, i / PTRS_IN_CACHELINE, 1);
      bitarray_write(pagePool, i / PTRS_IN_PAGE, 1);
      memPool[i] = CHILD_DATA;
    } else {
      memPool[i] = NORMAL_DATA;
    }
  }

  // Report dirty rates.
  ssize_t dirtyPage = 0;
  for(ssize_t i=0; i<pageNum; i++) {
    if(bitarray_read(pagePool, i))
      dirtyPage++;
  }
  printf("Dirty pages, %zd/%zu.\n", dirtyPage, pageNum);
  ssize_t dirtyCacheline = 0;
  for(ssize_t i=0; i<cachelineNum; i++) {
    if(bitarray_read(cachelinePool, i))
      dirtyCacheline++;
  }
  printf("Dirty cache lines, %zd/%zu.\n", dirtyCacheline, cachelineNum);
  ssize_t dirtyPtr = 0;
  for(ssize_t i=0; i<ptrNum; i++) {
    if(bitarray_read(vecbinPool, i))
      dirtyPtr++;
  }
  printf("Dirty ptrs, %zd/%zu.\n", dirtyPtr, ptrNum);

  size_t cntStart = 0;
  size_t cntTotal = 0;

  size_t ptrIdx;
  for(int iter=0; iter<ITER; iter++) {
    // Each time we trash the cache for a cold start.
    trash_cache(cacheTrashPool);
    ptrIdx = 0;
    cntStart = rdtsc_read();
    for(ssize_t i=0; i<cachelineNum; i++) {
      sweep_line(memPool+ptrIdx, MOM_DATA);
      ptrIdx += PTRS_IN_CACHELINE;
    }
    cntTotal += rdtsc_read() - cntStart;
  }
  printf("%ld cycles elapsed, no tricks.\n", cntTotal/ITER);

  cntStart = 0;
  cntTotal = 0;

  for(int iter=0; iter<ITER; iter++) {
    trash_cache(cacheTrashPool);
    ptrIdx = 0;
    cntStart = rdtsc_read();
    for(ssize_t pagei=0; pagei<pageNum; pagei++) {
      // If this page is dirty, scan it, otherwise skip.
      if(bitarray_read(pagePool, pagei)) {
        multi_tag_read();
        for(ssize_t i=0; i<CACHELINES_IN_PAGE; i++) {
          if(vecbinPool[ptrIdx/PTRS_IN_CACHELINE]) {
            // There is at least one pointer in this cache line, scan.
            sweep_line(memPool+ptrIdx, MOM_DATA);
          }
          ptrIdx += PTRS_IN_CACHELINE;
        }
      } else {
        ptrIdx += PTRS_IN_PAGE;
      }
    }
    cntTotal += rdtsc_read() - cntStart;
  }
  printf("%ld cycles elapsed, all tricks.\n", cntTotal/ITER);

  return 0;
}

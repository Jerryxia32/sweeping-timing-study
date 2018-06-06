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
  if(argc != 2) {
    fprintf(stderr, "Need just one argument, the filename of the vecbin.\n");
    exit(-1);
  }

  char* vecbinName = argv[1];
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

  size_t cntStart = rdtsc_read();

  size_t ptrIdx = 0;
  for(ssize_t i=0; i<cachelineNum; i++) {
    sweep_line(memPool+ptrIdx, MOM_DATA);
    ptrIdx += PTRS_IN_CACHELINE;
  }
  printf("%ld cycles elapsed, no tricks.\n", rdtsc_read() - cntStart);

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
  printf("%ld cycles elapsed, all tricks.\n", rdtsc_read() - cntStart);

  return 0;
}

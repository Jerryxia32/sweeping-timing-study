#ifndef _GADGET_H
#define _GADGET_H

#define MOM_DATA (0x10000120UL)
#define CHILD_DATA (0x7ffff120UL)
#define NORMAL_DATA (0x11000120UL)

#define PTR_SIZE (8) // size of a pointer, 8 bytes
#define CACHELINE_SIZE (64) // size of a cache line, 64 bytes
#define PAGE_SIZE (4096) // size of a page, 4KiB
#define CACHELINES_IN_PAGE (PAGE_SIZE/CACHELINE_SIZE)
#define PTRS_IN_CACHELINE (CACHELINE_SIZE/PTR_SIZE)
#define PTRS_IN_PAGE (PAGE_SIZE/PTR_SIZE)

#define CACHE_TRASH_SIZE (128UL<<20)

#include<inttypes.h>
#include<unistd.h>

static inline int __attribute__((always_inline))
x86_testsubset(uint64_t mom, uint64_t child) {
  return((mom > child)? 1:0);
}

extern void multi_tag_read();

static inline size_t __attribute__((always_inline))
rdtsc_read(void) {
  uint32_t hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return((size_t)lo | (((size_t)hi)<<32));
}

static inline int __attribute__((always_inline))
bitarray_read(char* bitarray, ssize_t idx) {
  ssize_t byteidx = idx / 8;
  ssize_t bitidx = idx % 8;
  if(bitarray[byteidx] & (1<<bitidx))
    return 1;
  else
    return 0;
}

static inline void __attribute__((always_inline))
bitarray_write(char* bitarray, ssize_t idx, int data) {
  ssize_t byteidx = idx / 8;
  ssize_t bitidx = idx % 8;
  char temp = bitarray[byteidx];
  if(data) {
    temp |= (1<<bitidx);
  } else {
    temp &= ~(1<<bitidx);
  }
  bitarray[byteidx] = temp;
}

void sweep_line(uint64_t* memPool, uint64_t momData);
void trash_cache(volatile uint64_t* cacheTrashPool);

#endif // _GADGET_H

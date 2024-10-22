// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  int size;
} kmems[NCPU];

void
kinit()
{
  for (int i = 0; i < NCPU; i++){
    initlock(&kmems[i].lock, "kmem");
    kmems[i].freelist = 0;
    kmems[i].size = 0;
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  push_off();
  int cpuidx = cpuid();
  pop_off();

  r = (struct run*)pa;
  acquire(&kmems[cpuidx].lock);
  r->next = kmems[cpuidx].freelist;
  kmems[cpuidx].freelist = r;
  kmems[cpuidx].size++;
  release(&kmems[cpuidx].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  struct run *tmpr;
  int max = 0;
  int maxidx = -1;
  push_off();
  int cpuidx = cpuid();
  pop_off();
  acquire(&kmems[cpuidx].lock);
  if (kmems[cpuidx].size == 0){
    for (int ci = 0; ci < NCPU; ci++){
      if (kmems[ci].size > max){
        max = kmems[ci].size;
        maxidx = ci;
      }
    }
    if (max > 0){
      acquire(&kmems[maxidx].lock);
      for (int a = 0; a < max / 2 + 1; a++){
        tmpr = kmems[maxidx].freelist;
        kmems[maxidx].freelist = tmpr->next;
        tmpr->next = kmems[cpuidx].freelist;
        kmems[cpuidx].freelist = tmpr;
        kmems[maxidx].size--;
        kmems[cpuidx].size++;
      }
      release(&kmems[maxidx].lock);
    } else {
      release(&kmems[cpuidx].lock);
      return 0;
    }
  }
  if (kmems[cpuidx].size < 1)
    panic("mem size < 1 after split mem");
  r = kmems[cpuidx].freelist;
  kmems[cpuidx].freelist = r->next;
  kmems[cpuidx].size--;
  release(&kmems[cpuidx].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

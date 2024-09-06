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
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  // if ((uint64)&(kmem.freelist) < KERNBASE)
  //   panic("kinit: &(kmem.freelist) < KERNBASE");
  // printf("&(kmem.freelist): %p\n", &(kmem.freelist));
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  // printf("end :%p\n", end);
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    // printf("page: %p\n", p);
    kfree(p);
  }
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
  if (!update_pagerefcount((uint64)pa,-1)){
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    if(update_pagerefcount((uint64)r, 0) != 0)
      panic("update_pagerefcount((uint64)r, 0) != 0");
    set_pagerefcount((uint64)r, 1); 
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}


static uint8 pgrefcount[(PHYSTOP - KERNBASE) >> PGSHIFT] = {0};

int
update_pagerefcount(uint64 pa, int n)
{
  

  if (n == -1){
    if (pgrefcount[(pa- KERNBASE) >> PGSHIFT] == 0)
      return 0;
    else if (pgrefcount[(pa- KERNBASE) >> PGSHIFT] < 0)
      panic("pgrefcount[pa >> PGSHIFT] < 0");
  }

  pgrefcount[(pa- KERNBASE) >> PGSHIFT] += (uint8)n;
  return pgrefcount[(pa- KERNBASE) >> PGSHIFT];
}

void
set_pagerefcount(uint64 pa, int n)
{
  pgrefcount[(pa- KERNBASE) >> PGSHIFT] = (uint8)n;
}

void
print_pagerefcount()
{
  for(uint64 i = 0; i < (PHYSTOP - KERNBASE) >> PGSHIFT; i++)
    if (pgrefcount[i] == 0)
      printf("page[%p]: %d\n", (i << PGSHIFT) + KERNBASE, pgrefcount[i]);
}

void
countfreepages()
{
  int fps = 0;
  for(uint64 i = ((uint64)end  - KERNBASE) >> PGSHIFT; i < (PHYSTOP - KERNBASE) >> PGSHIFT; i++)
    if (pgrefcount[i] == 0)
      fps++;
  printf("countfreepages(): free pages: %d\n", fps);
}
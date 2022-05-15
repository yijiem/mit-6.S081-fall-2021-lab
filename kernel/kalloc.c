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

// 128*1024*1024 / 4096 = 32768
int refcounts[32768];
#define INDEX(a) ((PGROUNDDOWN((uint64)a)-PGROUNDUP((uint64)end))/PGSIZE)

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  // initialize to 1 since kfree in freerange will decrease them back to 0
  for(int i = 0; i < 32768; i++)
    refcounts[i] = 1;
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

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&kmem.lock);
  if(refcounts[INDEX(pa)] <= 0)
    panic("kfree");

  refcounts[INDEX(pa)]--;
  if(refcounts[INDEX(pa)] > 0){
    release(&kmem.lock);
    return;
  }
  // only free when refcount drops to 0
  release(&kmem.lock);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
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
    refcounts[INDEX(r)] = 1;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void
increase_ref(void *pa)
{
  if((char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("increase_ref");

  acquire(&kmem.lock);
  if(refcounts[INDEX(pa)] <= 0)
    panic("increase_ref");

  refcounts[INDEX(pa)]++;
  release(&kmem.lock);
}

int
try_decrease_ref(void *pa)
{
  if((char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("try_decrease_ref");

  acquire(&kmem.lock);
  if(refcounts[INDEX(pa)] <= 1){
    release(&kmem.lock);
    return 0;
  }

  refcounts[INDEX(pa)]--;
  release(&kmem.lock);
  return 1;
}

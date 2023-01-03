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

struct kmem {
  char name[5];
  struct spinlock lock;
  struct run *freelist;
};

// per-cpu freelist
struct kmem kmems[NCPU];

void
kinit()
{
  for (int i = 0; i < NCPU; i++) {
    snprintf(kmems[i].name, 5, "kmem%d", i);
    initlock(&kmems[i].lock, kmems[i].name);
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

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  push_off();
  int id = cpuid();
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmems[id].lock);
  r->next = kmems[id].freelist;
  kmems[id].freelist = r;
  release(&kmems[id].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  push_off();
  int id = cpuid();
  struct run *r;

  acquire(&kmems[id].lock);
  r = kmems[id].freelist;
  if(r)
    kmems[id].freelist = r->next;
  release(&kmems[id].lock);

  if(!r) {
    // steal a page from another CPU's freelist
    for(int i = 0; i < NCPU; i++) {
      acquire(&kmems[i].lock);
      r = kmems[i].freelist;
      if(r) {
        // steal!
        kmems[i].freelist = r->next;
        release(&kmems[i].lock);
        break;
      }
      release(&kmems[i].lock);
    }
  }
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

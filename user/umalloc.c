// #include "types.h"
#include <stdint.h>
#include "../kernel/stat.h"
#include "user.h"
// #include "param.h"

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

// Each heap block starts with a Header.  The user pointer returned by
// malloc points just after this header.
union header {
  struct {
    // Next free block in the circular free list.
    union header *ptr;
    // Block size in Header-sized units, including this header.
    uint64_t size;
  } s;
  // Force Header to be aligned well enough for ordinary data.
  Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

void
free(void *ap)
{
  Header *bp, *p;

  // ap points at user data; the block header is immediately before it.
  bp = (Header*)ap - 1;

  // Find the place where bp belongs in the address-sorted circular free list.
  // The second condition handles the wraparound point at the end of memory.
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;

  // Merge with the next block if they are adjacent.
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;

  // Merge with the previous block if they are adjacent.
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}

static Header*
morecore(uint64_t nu)
{
  char *p;
  Header *hp;

  // Ask the kernel for memory in reasonably large chunks.
  if(nu < 4096)
    nu = 4096;
  p = sbrk(nu * sizeof(Header));
  if(p == (char*)-1)
    return 0;

  // Turn the newly grown heap region into a free block, then insert it
  // through free() so normal coalescing rules apply.
  hp = (Header*)p;
  hp->s.size = nu;
  free((void*)(hp + 1));
  return freep;
}

void*
malloc(uint64_t nbytes)
{
  Header *p, *prevp;
  uint64_t nunits;

  // Round the requested byte count up to Header units, plus one unit for
  // the block header itself.
  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;

  // First malloc call: create an empty circular list with base as sentinel.
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }

  // First-fit search through the circular free list.
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    if(p->s.size >= nunits){
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        // Allocate from the tail end of a larger free block.  The front
        // remains in the free list with the reduced size.
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      // Hide the header from the caller.
      return (void*)(p + 1);
    }
    if(p == freep)
      // No block was large enough; grow the heap and try again.
      if((p = morecore(nunits)) == 0)
        return 0;
  }
}

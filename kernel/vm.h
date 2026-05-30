#pragma once
#include <stdint.h>
struct proc;
struct inode;
typedef uint64_t pte_t;

pte_t* setup_kvm(void);
void switch_kvm(void);
void kvmalloc(void);
static void direct_map_init(pte_t* pml4);
static void kernel_map_init(pte_t *pml4);
void seg_init(void);
void* io_remap(void* pa, uint64_t size);



pte_t* setup_uvm(void);
void init_uvm(pte_t *pml4, char *init, uint64_t sz);
void switch_uvm(struct proc *p);
pte_t* get_kpml4(void);
pte_t* copy_uvm(pte_t *pml4, uint64_t sz);
void free_vm(pte_t *pml4, uint64_t sz);
uint64_t alloc_uvm(pte_t *pml4, uint64_t old_sz, uint64_t new_sz);
int load_uvm(pte_t *pml4, char *addr, struct inode *ip, uint64_t offset, uint64_t sz) ;
void clear_pte_u(pte_t *pml4, char *uva);
int copy_out(pte_t *pml4, uint64_t va, void* p, uint64_t len);
char* uva2dma(pte_t *pml4, char *uva);
uint64_t dealloc_uvm(pte_t *pml4, uint64_t old_sz, uint64_t new_sz) ;

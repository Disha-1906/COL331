#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"

struct swap_slot{
  uint page_perm;
  uint is_free;
  uint start; //block id of start
};


static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}


struct swap_slot swap_array[SWAP_SLOT];
#define SWAP_SIZE   8

void swap_space_init(){
    for(int i=0;i<SWAP_SLOT;++i){
        swap_array[i].is_free = 1;
        swap_array[i].page_perm = 0;
        swap_array[i].start = SWAP_START + i * SWAP_SIZE;
    }
}

int get_swap_slot(){
    for(int i=0;i<SWAP_SLOT;++i){
        if(swap_array[i].is_free == 1){
            swap_array[i].is_free = 0;
            swap_array[i].page_perm = 0;
            return i;
        }
    }
    return -1;
}


pte_t* select_a_victim_page(struct proc* v_process){
    pde_t* pgdir = v_process->pgdir;
    for(int i=0;i<NPDENTRIES;i++){
        if(!(pgdir[i] & PTE_P)){
            continue;
        }
        pte_t* pgtab = (pte_t*)P2V(PTE_ADDR(pgdir[i]));
        for(int j=0;j<NPTENTRIES;j++){
            if((pgtab[j]&PTE_P)&& (pgtab[j]&PTE_U)){
                if(!(pgtab[j]&PTE_A)){
                    return &pgtab[j];
                }
            }
        }
    }
    
    int count=0;
    for(int i=0;i<NPDENTRIES;i++){
        if (!(pgdir[i] & PTE_P)) continue;
        pte_t* pgtab = (pte_t*)P2V(PTE_ADDR(pgdir[i]));
        for(int j=0;j<NPTENTRIES;j++){
            if((pgtab[j]&PTE_P)&& (pgtab[j]&PTE_U) && (pgtab[j] & PTE_A)){
                if(count%10==0) pgtab[j] &= ~PTE_A;
            count++;
            }
        }
    }
    return select_a_victim_page(v_process);
}

void swap_out_page(pte_t *pte){
    uint physical_address = PTE_ADDR(*pte);
    int slot_number = get_swap_slot();
    if(slot_number==-1){
        panic("no swap slot available");
    }
    write_page_to_disk(ROOTDEV, (char*)P2V(physical_address), swap_array[slot_number].start);
    swap_array[slot_number].page_perm = PTE_FLAGS(*pte);
    *pte = PTE_FLAGS(*pte) | (swap_array[slot_number].start << PTXSHIFT);
    *pte = *pte & ~PTE_P;
    *pte |= PTE_SP;
    kfree((char*)P2V(physical_address));
}

// swap page out by finding victim page and swapping that page
void swap_page(){
    struct proc* p = select_victim_process();
    pte_t* pte = select_a_victim_page(p);
    p->rss -= PGSIZE;
    swap_out_page(pte);
}

void swap_page_in(){
    struct proc* p = myproc();
    uint fault_addr = rcr2();
    pte_t* fault_page = walkpgdir(p->pgdir, (char*)fault_addr, 0); 
    int block = (*fault_page) >> PTXSHIFT;
    char* new_page_addr = kalloc();
    p->rss += PGSIZE;
    read_page_from_disk(ROOTDEV, new_page_addr, block);
    int swap_idx = (block-2)/8;
    uint perm = swap_array[swap_idx].page_perm;
    *fault_page = V2P(new_page_addr) | perm;
    *fault_page |= PTE_P;
    swap_array[swap_idx].is_free = 1;
    swap_array[swap_idx].page_perm = 0;
}


void clear_swap_slot(int slot){
    swap_array[slot].is_free = 1;
    swap_array[slot].page_perm = 0;
}



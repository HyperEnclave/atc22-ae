#ifndef _PAGING_H_
#define _PAGING_H_

#include <stdint.h>
#include "Enclave.h"

const uint64_t PHYS_OFFSET = 0xffff800000000000;

typedef uint64_t pte_t;

#define PGSIZE          4096    // bytes mapped by a page

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))

// Page table/directory entry flags.
#define PTE_P           0x001   // Present
#define PTE_W           0x002   // Writeable
#define PTE_U           0x004   // User
#define PTE_PWT         0x008   // Write-Through
#define PTE_PCD         0x010   // Cache-Disable
#define PTE_A           0x020   // Accessed
#define PTE_D           0x040   // Dirty
#define PTE_PS          0x080   // Page Size
#define PTE_G           0x100   // Global
#define PTE_NX          (1ll << 63)   // No execute

#define P4_IDX(vaddr)     (((uint64_t)(vaddr) >> (12 + 27)) & 0777)
#define P3_IDX(vaddr)     (((uint64_t)(vaddr) >> (12 + 18)) & 0777)
#define P2_IDX(vaddr)     (((uint64_t)(vaddr) >> (12 + 9)) & 0777)
#define P1_IDX(vaddr)     (((uint64_t)(vaddr) >> 12) & 0777)

// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint64_t)(pte) & 0xFFFFFFFFF000)
#define PTE_FLAGS(pte)  ((uint64_t)(pte) & ~0xFFFFFFFFF000)

static uint64_t g_cached_cr3;

static inline uint64_t read_cr2()
{
    uint64_t ret;
    asm volatile("mov %%cr2, %0" : "=r" (ret));
    return ret;
}

static inline uint64_t read_cr3()
{
    uint64_t ret;
    asm volatile("mov %%cr3, %0" : "=r" (ret));
    return ret;
}

static inline void write_cr3(uint64_t val)
{
    asm volatile("mov %0, %%cr3" : : "r" (val));
}

static inline void cache_cr3(uint64_t cr3) {
    g_cached_cr3 = cr3;
}

static inline void* phys_to_virt(uint64_t paddr)
{
    return (void*)(paddr + PHYS_OFFSET);
}

static inline  pte_t* next_table(pte_t pte)
{
    if (!(pte & PTE_P)) {
        return NULL;
    } else if (pte & PTE_PS) {
        abort();
    } else {
        return (pte_t*)phys_to_virt(PTE_ADDR(pte));
    }
}

static inline pte_t* get_entry(uint64_t vaddr)
{
    pte_t* p4 = (pte_t*)phys_to_virt(g_cached_cr3);
    pte_t p4e = p4[P4_IDX(vaddr)];

    pte_t* p3 = next_table(p4e);
    if (!p3) {
        return NULL;
    }
    pte_t p3e = p3[P3_IDX(vaddr)];

    pte_t* p2 = next_table(p3e);
    if (!p2) {
        return NULL;
    }
    pte_t p2e = p2[P2_IDX(vaddr)];

    pte_t* p1 = next_table(p2e);
    if (!p1) {
        return NULL;
    }
    return &p1[P1_IDX(vaddr)];
}

static inline int mprotect_page(uint64_t vaddr, uint64_t flags)
{
    pte_t* pte = get_entry(vaddr);
    // printf_ocall("%llx %p\n", vaddr, pte);
    if (!pte) {
        return -1;
    }
    *pte = PTE_ADDR(*pte) | flags;
    asm volatile("invlpg (%0)" :: "r"(vaddr));
    return 0;
}

int mprotect(void *addr, size_t len, uint64_t flags)
{
    uint64_t start_vaddr = (uint64_t)addr;
    // printf_ocall("%llx %d\n", start_vaddr, len);
    if (start_vaddr % PGSIZE != 0 || len % PGSIZE != 0) {
        return -1;
    }
    uint64_t end_vaddr = start_vaddr + len;
    while (start_vaddr < end_vaddr) {
        mprotect_page(start_vaddr, flags);
        start_vaddr += PGSIZE;
    }
    return 0;
}

static void dump_inner(uint64_t table_paddr, uint64_t start_vaddr, int level, int limit)
{
    int n = 0;
    pte_t* table = (uint64_t*)phys_to_virt(table_paddr);
    for (int i = 0; i < 512; i++)
    {
        uint64_t vaddr = start_vaddr + ((uint64_t)i << (12 + (3 - level) * 9));
        if ((vaddr & (1ll << 47)) != 0) {
            vaddr |= ~((1ll << 47) - 1);
        }
        pte_t pte = table[i];
        if (pte != 0) {
            printf_ocall("%*s[%d - %03x], 0x%016llx -> 0x%016llx\n", level*2, " ", level, i, vaddr, pte);
            if (level < 3) {
                if (!(pte & PTE_PS))
                    dump_inner(PTE_ADDR(pte), vaddr, level + 1, limit);
            }
            n += 1;
            if (n >= limit) {
                break;
            }
        }
    }
}

static void dump_pgtable(uint64_t root_paddr, int limit)
{
    dump_inner(root_paddr, 0, 0, limit);
}

#endif // _PAGING_G_

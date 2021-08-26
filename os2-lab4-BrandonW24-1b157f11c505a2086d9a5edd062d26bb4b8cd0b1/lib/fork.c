// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{

  void *addr = (void*)utf->utf_fault_va;
  uint32_t err = utf->utf_err;
  int r;

  if (!(err & FEC_WR) || !(uvpt[(uint32_t)addr / PGSIZE] & PTE_COW))
    	panic("PGFAULT PANIC : ATTEMPTING TO ACCESS %e", err);

  if ((r = sys_page_alloc(0, PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
    	panic("SYS_PAGE_ALLOC PANIC MESSAGE INVOKED : %e", r);

  memmove(PFTEMP, (void*)((uint32_t)addr / PGSIZE * PGSIZE), PGSIZE);

  if ((r = sys_page_map(0, PFTEMP, 0, (void*)((uint32_t)addr / PGSIZE * PGSIZE), PTE_P|PTE_U|PTE_W)) < 0)
    	panic("SYS_PAGE_MAP PANIC MESSAGE INVOKED : %e", r);

  if ((r = sys_page_unmap(0, PFTEMP)) < 0)
    	panic("SYS_PAGE_UNMAP PANIC MESSAGE INVOKED : %e", r);

}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
  int r;

  if (uvpt[pn] & PTE_W || uvpt[pn] & PTE_COW) {
    if ((r = sys_page_map(0, (void*)(pn * PGSIZE), envid,(void*)(pn * PGSIZE), PTE_P | PTE_U | PTE_COW)) < 0)
        return r;

    // set PTE_COW bit for pgtable
    if ((r = sys_page_map(envid, (void*)(pn * PGSIZE), 0,(void*)(pn * PGSIZE), PTE_P | PTE_U | PTE_COW)) < 0)
        return r;

  }else{
    // if it is a read-only page, just copy the mapping
    if((r = sys_page_map(0, (void*)(pn * PGSIZE), envid,(void*)(pn * PGSIZE), uvpt[pn] & 0xfff)) < 0){
      cprintf("sys_page_map failed\n");
      return r;
    }
  }
  return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void){

  envid_t envid;
  int r;
  uint32_t i, j, pn;
  extern volatile pte_t uvpt[];
  extern volatile pde_t uvpd[];
  extern char end[];
  
  if (!thisenv->env_pgfault_upcall){
	set_pgfault_handler(pgfault);
  }
    	
  envid = sys_exofork();

  if (envid < 0){
	panic("SYS_EXOFORK FAILED, PANIC MESSAGE INVOKED : %e", envid);
  }
 
  if (envid == 0) {
    thisenv = &envs[ENVX(sys_getenvid())];
    return 0;
  }

  for (i = 0; i < NPDENTRIES; i++) {
    for (j = 0; j < NPTENTRIES; j++) {
      pn = i * NPDENTRIES + j;
      if (pn * PGSIZE < UTOP && uvpd[i] && uvpt[pn]&& (pn * PGSIZE != UXSTACKTOP - PGSIZE)){
        if ((r = duppage(envid, pn)))
          cprintf("duppage: %e\n", r);
      }
    }
  }

  if ((r = sys_page_alloc(envid, (void*)(UXSTACKTOP - PGSIZE), PTE_P|PTE_U|PTE_W)) < 0)
    	panic("SYS_PAGE_ALLOC PANIC MESSAGE OCCURRED : %e", r);

  if ((r = sys_page_map(envid, (void*)(UXSTACKTOP - PGSIZE), 0, PFTEMP, PTE_P|PTE_U|PTE_W)) < 0)
    	panic("SYS_PAGE_MAP PANIC MESSAGE OCCURRED: %e", r);

  memmove(PFTEMP, (void*)(UXSTACKTOP - PGSIZE), PGSIZE);

  if ((r = sys_page_unmap(0, PFTEMP)) < 0)
    	panic("SYS_PAGE_UNMAP PANIC MESSAGE OCCURRED: %e", r);

  sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);
  sys_env_set_status(envid, ENV_RUNNABLE);
  return envid;

}
// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}

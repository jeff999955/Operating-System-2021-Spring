#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"


uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

//void *sys_mmap(void *addr, uint length, int prot, int flags, int fd, int offset){
uint64 sys_mmap(void){
  struct proc *p = myproc();
  uint64 addr;
  int length, prot, flags, fd, offset;

  if(argaddr(0, &addr) < 0 || argint(1, &length) < 0 || argint(2, &prot) < 0 || argint(3, &flags) < 0
    || argint(4, &fd) < 0 || argint(5, &offset) < 0)
    return -1;

  if(addr != 0 || offset != 0)
    return -1;

  if(p->ofile[fd] == 0 || p->ofile[fd]->type != FD_INODE)
    return -1;

  struct file *f = p->ofile[fd];
  if(f < 0 || ( (flags & MAP_SHARED) && (prot & PROT_WRITE) && !f->writable))
    return -1;

  struct VMA *vma = 0;
  for (int i = 0; i < NVMA; ++i) 
    if (p->vma[i].vm_valid) {vma = &(p->vma[i]); break;}
  
  vma->vm_start = PGROUNDUP(p->current_heap - length); // lower i.e. smaller
  vma->vm_end = PGROUNDUP(p->current_heap); // higher
  vma->vm_prot = prot;
  vma->vm_flags = flags;
  vma->vm_file = f;
  vma->vm_valid = 0;
  filedup(vma->vm_file); // found in file.c, thread-safe function to increment ref count
  p->current_heap = vma->vm_start;

  return vma->vm_start;
}

//int sys_munmap(void *addr, uint length) {
uint64 sys_munmap(void){
  struct proc *p = myproc();
  uint64 addr;
  int length;
  if (argaddr(0, &addr) < 0 || argint(1, &length) < 0)
    return -1;
  // assume always unmaps for the starm of a vma till its end
  // then we don't have to split the vma
  struct VMA *vma = 0;
  for (int i = 0; i < NVMA; ++i) {
    if (p->vma[i].vm_start <= addr && addr <= p->vma[i].vm_end) {
      vma = &(p->vma[i]);
      break;
    }
  }
  if (!vma || vma->vm_valid) return -1;
  uint64 pa; // find the physical address
  for (uint64 va = vma->vm_start; va < addr + length; va += PGSIZE) {
    if ((pa = walkaddr(p->pagetable, va))) {
      if (vma->vm_flags == MAP_SHARED)
        filewrite(vma->vm_file, va, PGSIZE);
      uvmunmap(p->pagetable, va, 1, 1);
    }
  }
  vma->vm_start += length;
  if (vma->vm_start == vma->vm_end) {
    fileclose(vma->vm_file);
    vma->vm_valid = 1;
  }
  return 0;
}

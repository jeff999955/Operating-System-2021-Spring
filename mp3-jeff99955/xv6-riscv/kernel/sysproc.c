#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

void restore_content(void) {
    struct proc *p = myproc();
    struct thrd_context_data *tcd = &(p->thrdstop_context[p->thrdstop_context_id]);
    struct trapframe *tf = p->trapframe;
    tf->s0 = tcd->s_regs[0];
    tf->s1 = tcd->s_regs[1];
    tf->s2 = tcd->s_regs[2];
    tf->s3 = tcd->s_regs[3];
    tf->s4 = tcd->s_regs[4];
    tf->s5 = tcd->s_regs[5];
    tf->s6 = tcd->s_regs[6];
    tf->s7 = tcd->s_regs[7];
    tf->s8 = tcd->s_regs[8];
    tf->s9 = tcd->s_regs[9];
    tf->s10 = tcd->s_regs[10];
    tf->s11 = tcd->s_regs[11];
    tf->ra = tcd->ra;
    tf->sp = tcd->sp;
    tf->t0 = tcd->t_regs[0];
    tf->t1 = tcd->t_regs[1];
    tf->t2 = tcd->t_regs[2];
    tf->t3 = tcd->t_regs[3];
    tf->t4 = tcd->t_regs[4];
    tf->t5 = tcd->t_regs[5];
    tf->t6 = tcd->t_regs[6];
    tf->a0 = tcd->a_regs[0];
    tf->a1 = tcd->a_regs[1];
    tf->a2 = tcd->a_regs[2];
    tf->a3 = tcd->a_regs[3];
    tf->a4 = tcd->a_regs[4];
    tf->a5 = tcd->a_regs[5];
    tf->a6 = tcd->a_regs[6];
    tf->a7 = tcd->a_regs[7];
    tf->gp = tcd->gp;
    tf->tp = tcd->tp;
    tf->epc = tcd->epc;
    return;
}

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


// for mp3
uint64
sys_thrdstop(void)
{
  int interval, thrdstop_context_id;
  uint64 handler;
  if (argint(0, &interval) < 0)
    return -1;
  if (argint(1, &thrdstop_context_id) < 0)
    return -1;
  if (argaddr(2, &handler) < 0)
    return -1;
	struct proc *cp = myproc();
  cp->thrdstop_ticks = 0;
    cp->thrdstop_interval = interval;
    cp->thrdstop_handler_pointer = handler;
    if (thrdstop_context_id < 0) {
        for (int i = MAX_THRD_NUM - 1; i >= 0; --i)
            if (!cp->thrdstop_context_used[i]) {
                cp->thrdstop_context_used[i] = 1;
                cp->thrdstop_context_id = i;
                return i;
            }
        return -1;
    } else {
      cp->thrdstop_context_used[thrdstop_context_id] = 1;
      cp->thrdstop_context_id = thrdstop_context_id;
    }
    return thrdstop_context_id;
}

// for mp3
uint64
sys_cancelthrdstop(void)
{
  int thrdstop_context_id;
  if (argint(0, &thrdstop_context_id) < 0)
    return -1;
   struct proc *cp = myproc();
    cp->thrdstop_interval = -1;
    int ret = cp->thrdstop_ticks;
    int i = thrdstop_context_id;
    if (i >= 0 && cp->thrdstop_context_used[i])
        move_content();
    return ret;
}

// for mp3
uint64
sys_thrdresume(void)
{
  int  thrdstop_context_id, is_exit;
  if (argint(0, &thrdstop_context_id) < 0)
    return -1;
  if (argint(1, &is_exit) < 0)
    return -1;
  struct proc *cp = myproc();
    int i = thrdstop_context_id;
    if (is_exit) {
        cp->thrdstop_context_used[i] = 0;
        cp->thrdstop_interval = -1;
    } else {
        restore_content();
    }

  return 0;
}

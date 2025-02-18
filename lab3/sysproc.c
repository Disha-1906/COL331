#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

/* System Call Definitions */
int 
set_sched_policy(void)
{
    // Implement your code here 
    struct proc* curr_proc = myproc();
    int *ip =0; 
    int pol = argint(1,ip);
    if (pol==0 && (*ip==0 || *ip==1)){
        curr_proc->policy= *ip;
        return 0;
    }
    return -22;
}

int 
get_sched_policy(void)
{
    // Implement your code here 
    struct proc* curr_proc = myproc();
    return curr_proc->policy;
    // return -1;
}

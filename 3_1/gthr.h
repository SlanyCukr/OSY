#ifndef __GTHR_H
#define __GTHR_H

#include <stdint.h>
#include <time.h>

#ifndef GT_PREEMPTIVE 

#define GT_PREEMPTIVE 1

#endif

enum {
    MaxGThreads = 5,                // Maximum number of threads, used as array size for gttbl
    StackSize = 0x400000,           // Size of stack of each thread
    TimePeriod = 10,                // Time period of Timer
};

// Thread state
typedef enum {
    Unused,
    Running,
    Ready,
    Blocked,
    Suspended,
} gt_thread_state_t;


struct gt_context_t {
    struct gt_regs {                // Saved context, switched by gt_*swtch.S (see for detail)
        uint64_t rsp;
        uint64_t rbp;
#if ( GT_PREEMPTIVE == 0 )
        uint64_t r15;
        uint64_t r14;
        uint64_t r13;
        uint64_t r12;
        uint64_t rbx;
#endif
    }
    regs;

    gt_thread_state_t thread_state; // process state
    const char* name;               // process name
    int ticks;                      // num of ticks for this thread
};

void gt_task_suspend(int index);
void gt_task_resume(int index);
void gt_task_list();
const char* gt_getname();
int gettid();
void gt_init( void );                       // initialize gttbl
int  gt_go( void ( * t_run )( void ), const char* name );     // create new thread and set f as new "run" function
void gt_stop( void );                       // terminate current thread
int  gt_yield( void );                      // yield and switch to another thread
void gt_scheduler( void );                  // start scheduler, wait for all tasks

void gt_ret( int t_ret );                   // terminate thread

#if ( GT_PREEMPTIVE == 0 )
void gt_swtch( struct gt_regs * t_old, struct gt_regs * t_new );        // declaration from gtswtch.S
#else
void gt_pree_swtch( struct gt_regs * t_old, struct gt_regs * t_new );   // declaration from gtswtch.S
#endif

int uninterruptibleNanoSleep( time_t sec, long nanosec );   // uninterruptible sleep

#endif // __GTHR_H

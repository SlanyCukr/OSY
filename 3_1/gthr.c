
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "time.h"

#include "gthr.h"

struct gt_context_t g_gttbl[ MaxGThreads ]; // statically allocated table for thread control
struct gt_context_t * g_gtcur;              // pointer to current thread
struct timespec g_start_time;               // starting point in time for running thread

#if ( GT_PREEMPTIVE != 0 )

char thread_str_state[5][10] = {"Unused", "Running", "Ready", "Blocked", "Suspended"};

void gt_task_suspend(int index)
{
    if(index == -1)
        g_gtcur->thread_state=Suspended;
    else
    {
        struct gt_context_t* p = & g_gttbl[index];
        p->thread_state=Suspended;
    }
}
void gt_task_resume(int index)
{
    if(index == -1)
        g_gtcur->thread_state=Ready;
    else
    {
        struct gt_context_t* p = & g_gttbl[index];
        p->thread_state=Ready;
    }
}

void gt_task_list()
 {
     for(int i = 0; i < MaxGThreads; i++)
     {
         struct gt_context_t* p = & g_gttbl[i];
         printf("Name: %s Id: %d State:%s\n", p->name, i, thread_str_state[p->thread_state]);
     }
 }

const char* gt_getname()
{
    return g_gtcur->name;
}

int gettid()
{
    return g_gtcur - & g_gttbl[0];
}

void gt_sig_handle( int t_sig );

// initialize SIGALRM
void gt_sig_start( void )
{
    struct sigaction l_sig_act;
    bzero( &l_sig_act, sizeof( l_sig_act ) );
    l_sig_act.sa_handler = gt_sig_handle;

    sigaction( SIGALRM, &l_sig_act, NULL );
    ualarm( TimePeriod * 1000, TimePeriod * 1000 );
}

// unblock SIGALRM
void gt_sig_reset( void ) 
{
    sigset_t l_set;                     // Create signal set
    sigemptyset( & l_set );             // Clear it
    sigaddset( & l_set, SIGALRM );      // Set signal (we use SIGALRM)

    sigprocmask( SIG_UNBLOCK, & l_set, NULL );  // Fetch and change the signal mask
}

// function triggered periodically by timer (SIGALRM)
void gt_sig_handle( int t_sig ) 
{
    gt_sig_reset();                     // enable SIGALRM again

    // .... manage timers here
    struct timespec now;
    struct timespec result;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);

    result.tv_nsec = now.tv_nsec - g_start_time.tv_nsec;
    result.tv_sec = now.tv_sec - g_start_time.tv_sec;

    printf("%lld.%.9ld\n", (long long)result.tv_sec, result.tv_nsec);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &g_start_time);

    gt_yield();                         // scheduler
}

#endif

// initialize first thread as current context (and start timer)
void gt_init( void ) 
{
    g_gtcur = & g_gttbl[ 0 ];           // initialize current thread with thread #0
    g_gtcur -> thread_state = Running;  // set current to running

    // start with ticks 0
    for(int i = 0; i < MaxGThreads; i++)
    {
        struct gt_context_t* p = & g_gttbl[i];
        p->ticks = 0;
    }

#if ( GT_PREEMPTIVE != 0 )
    gt_sig_start();
#endif
}


// exit thread
void gt_ret( int t_ret ) 
{
    if ( g_gtcur != & g_gttbl[ 0 ] )    // if not an initial thread,
    {
        g_gtcur -> thread_state = Unused;// set current thread as unused
        gt_yield();                     // yield and make possible to switch to another thread
        assert( !"reachable" );         // this code should never be reachable ... (if yes, returning function on stack was corrupted)
    }
}

void gt_scheduler( void )
{
    while ( gt_yield() )                // if initial thread, wait for other to terminate
    {
        if ( GT_PREEMPTIVE )
            usleep( TimePeriod * 1000 );// idle, simulate CPU HW sleep
    }
}


// switch from one thread to other
int gt_yield( void ) 
{
    struct gt_context_t * p;
    struct gt_regs * l_old, * l_new;
    int l_no_ready = 0;                 // not ready processes

    p = g_gtcur;
    while ( p -> thread_state != Ready )// iterate through g_gttbl[] until we find new thread in state Ready 
    {
        if ( p -> thread_state == Blocked || p -> thread_state == Suspended ) 
            l_no_ready++;               // number of Blocked and Suspend processes

        if ( ++p == & g_gttbl[ MaxGThreads ] )// at the end rotate to the beginning
            p = & g_gttbl[ 0 ];
        if ( p == g_gtcur )             // did not find any other Ready threads
        {
            return - l_no_ready;        // no task ready, or task table empty
        }
    }

    if ( g_gtcur -> thread_state == Running )// switch current to Ready and new thread found in previous loop to Running
        g_gtcur -> thread_state = Ready;
    p -> thread_state = Running;
    l_old = & g_gtcur -> regs;          // prepare pointers to context of current (will become old) 
    l_new = & p -> regs;                // and new to new thread found in previous loop
    g_gtcur = p;                        // switch current indicator to new thread
#if ( GT_PREEMPTIVE != 0 )
    gt_pree_swtch( l_old, l_new );      // perform context switch (assembly in gtswtch.S)
#else
    gt_swtch( l_old, l_new );           // perform context switch (assembly in gtswtch.S)
#endif

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &g_start_time);

    return 1;
}


// return function for terminating thread
void gt_stop( void ) 
{
    gt_ret( 0 );
}


// create new thread by providing pointer to function that will act like "run" method
int gt_go( void ( * t_run )( void ), const char* name ) 
{
    char * l_stack;
    struct gt_context_t * p;
    
    for ( p = & g_gttbl[ 0 ];; p++ )            // find an empty slot
        if ( p == & g_gttbl[ MaxGThreads ] )    // if we have reached the end, gttbl is full and we cannot create a new thread
            return -1;
        else if ( p -> thread_state == Unused )
            break;                              // new slot was found

    l_stack = ( char * ) malloc( StackSize );   // allocate memory for stack of newly created thread
    if ( !l_stack )
        return -1;

    *( uint64_t * ) & l_stack[ StackSize - 8 ] = ( uint64_t ) gt_stop;  //  put into the stack returning function gt_stop in case function calls return
    *( uint64_t * ) & l_stack[ StackSize - 16 ] = ( uint64_t ) t_run;   //  put provided function as a main "run" function
    p -> regs.rsp = ( uint64_t ) & l_stack[ StackSize - 16 ];           //  set stack pointer
    p->name = name;                                                     //  set name
    p -> thread_state = Ready;                                          //  set state

    return p - & g_gttbl[0];
}


int uninterruptibleNanoSleep( time_t t_sec, long t_nanosec ) 
{
#if 0
    struct timeval l_tv_cur, l_tv_limit;
    struct timeval l_tv_tmp = { t_sec, t_nanosec / 1000 };
    gettimeofday( &l_tv_cur, NULL );
    timeradd( &l_tv_cur, &l_tv_tmp, &l_tv_limit );
    while ( timercmp( &l_tv_cur, &l_tv_limit, <= ) )
    {
        timersub( &l_tv_limit, &l_tv_cur, &l_tv_tmp );
        struct timespec l_tout = { l_tv_tmp.tv_sec, l_tv_tmp.tv_usec * 1000 }, l_res;    
        if ( nanosleep( & l_tout, &l_res ) < 0 )
        {
            if ( errno != EINTR )
                return -1;
        }
        else
        {
            break;
        }
        gettimeofday( &l_tv_cur, NULL );
    }

    // add time to global running time of this thread
    g_tv_cur.tv_sec += l_tv_cur.tv_sec;
    g_tv_cur.tv_usec += l_tv_cur.tv_usec;

    return 0;

#else
    struct timespec req;
    req.tv_sec = t_sec;
    req.tv_nsec = t_nanosec;
    
    do {
        if (0 != nanosleep( & req, & req)) {
        if (errno != EINTR)
            return -1;
        } else {
            break;
        }
    } while (req.tv_sec > 0 || req.tv_nsec > 0);
#endif
    return 0; /* Return success */
}

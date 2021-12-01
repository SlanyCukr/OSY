//***************************************************************************
//
// GThreads - preemptive threads in userspace. 
// Inspired by https://c9x.me/articles/gthreads/code0.html.
//
// Program created for subject OSMZ and OSY. 
//
// Michal Krumnikl, Dept. of Computer Sciente, michal.krumnikl@vsb.cz 2019
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2021
//
// Program simulates preemptice switching of user space threads. 
//
//***************************************************************************

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "gthr.h"

#define COUNT 100

// Dummy function to simulate some thread work
void f( void ) {
    static int x;
    int count = COUNT;
    int id = ++x;

    while ( count-- )
    {
        printf( "F Thread id: %d count: %d table_id: %d name: %s\n", id, count, gettid(), gt_getname() );
        gt_task_suspend(-1);
        uninterruptibleNanoSleep( 0, 1000000 );
#if ( GT_PREEMPTIVE == 0 )
        gt_yield();
#endif
    }
}

// Dummy function to simulate some thread work
void g( void ) {
    static int x = 0;
    int count = COUNT;
    int id = ++x;

    while ( count-- )
    {
        printf( "G Thread id: %d count: %d\n", id, count );
        uninterruptibleNanoSleep( 0, 1000000 );
#if ( GT_PREEMPTIVE == 0 )
        gt_yield();
#endif
    }
    gt_task_resume(1);
    gt_task_resume(2);
}

int main(void) {
    gt_init();      // initialize threads, see gthr.c

    gt_task_list();

    printf("gt_go f: %d\n", gt_go( f, "prvni" ));     // set f() as first thread
    printf("gt_go f: %d\n", gt_go( f , "druhe"));     // set f() as second thread
    printf("gt_go g: %d\n", gt_go( g, "treti" ));     // set g() as third thread
    printf("gt_go g: %d\n", gt_go( g, "ctvrte" ));     // set g() as fourth thread

    gt_task_list();

    gt_scheduler(); // wait until all threads terminate
  
    printf( "Threads finished\n" );
}

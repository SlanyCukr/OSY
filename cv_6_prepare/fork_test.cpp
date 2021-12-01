#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    
        pid_t pid = 0;
        int   status;
        int roura[2];
        pipe(roura);

        pid = fork();
        if (pid == 0) {
            // child
                close(roura[0]);
                dup2(roura[1], 1);
                close(roura[1]);

                printf("I am the child.\n");
                execlp("/bin/ls", "ls", nullptr); 
                perror("In exec(): ");
        }
        if (pid > 0) {
            // parent
            close(roura[1]);

            char l_buf[ 1024 ];
            int l_len = read( roura[ 0 ], l_buf, sizeof( l_buf ) );

            int l_ret = write( STDOUT_FILENO, l_buf, l_len );

            close(roura[0]);
            wait(nullptr);
        }

        exit(0);
}
//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// Example of socket server.
//
// This program is example of socket server and it allows to connect and serve
// the only one client.
// The mandatory argument of program is port number for listening.
//
//***************************************************************************

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#define STR_CLOSE   "close"
#define STR_QUIT    "quit"

//***************************************************************************
// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages
#define MAX_CLIENTS             3       // max num of clients

// debug flag
int g_debug = LOG_INFO;

void log_msg( int t_log_level, const char *t_form, ... )
{
    const char *out_fmt[] = {
            "ERR: (%d-%s) %s\n",
            "INF: %s\n",
            "DEB: %s\n" };

    if ( t_log_level && t_log_level > g_debug ) return;

    char l_buf[ 1024 ];
    va_list l_arg;
    va_start( l_arg, t_form );
    vsprintf( l_buf, t_form, l_arg );
    va_end( l_arg );

    switch ( t_log_level )
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf( stdout, out_fmt[ t_log_level ], l_buf );
        break;

    case LOG_ERROR:
        fprintf( stderr, out_fmt[ t_log_level ], errno, strerror( errno ), l_buf );
        break;
    }
}

int G_SOCKETS[3][2] = {{-1, -1}, {-1,-1}, {-1,-1}};

void write_to_clients(int cur_index, const char* message)
{
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(G_SOCKETS[i][0] == -1)
            return;

        // write data to client
        int l_len = write( G_SOCKETS[i][0], message, strlen(message) );
        if ( l_len < 0 )
            log_msg( LOG_ERROR, "Unable to write data to client." );
        }
}

void* handle_client(void* t_par)
{
    int index = (intptr_t)t_par;

    int l_sock_client = G_SOCKETS[index][0];
    int l_sock_listen = G_SOCKETS[index][1];

    // list of fd sources
    pollfd l_read_poll[ 2 ];

    l_read_poll[ 1 ].fd = l_sock_client;
    l_read_poll[ 1 ].events = POLLIN;

    while ( 1  )
        { // communication
            char l_buf[ 256 ];
            for(int i = 0; i < 256; i++)
                l_buf[i] = '\0';

            // select from fds
            int l_poll = poll( l_read_poll, 2, -1 );

            if ( l_poll < 0 )
            {
                log_msg( LOG_ERROR, "Function poll failed!" );
                exit( 1 );
            }

            // data from client?
            if ( l_read_poll[ 1 ].revents & POLLIN )
            {
                // read data from socket
                int l_len = read( l_sock_client, l_buf, sizeof( l_buf ) );
                if ( !l_len )
                {
                        log_msg( LOG_DEBUG, "Client closed socket!" );
                        close( l_sock_client );
                        break;
                }
                else if ( l_len < 0 )
                        log_msg( LOG_DEBUG, "Unable to read data from client." );
                else
                        log_msg( LOG_DEBUG, "Read %d bytes from client.", l_len );

                // // write data to client
                // l_len = write( l_sock_client, l_buf, l_len );
                // if ( l_len < 0 )
                //         log_msg( LOG_ERROR, "Unable to write data to client." );

                // writes to other clients
                write_to_clients(index, l_buf);

                // write data to stdout
                l_len = write( STDOUT_FILENO, l_buf, l_len );
                if ( l_len < 0 )
                        log_msg( LOG_ERROR, "Unable to write data to stdout." );

                // close request?
                if ( !strncasecmp( l_buf, "close", strlen( STR_CLOSE ) ) )
                {
                        log_msg( LOG_INFO, "Client sent 'close' request to close connection." );
                        close( l_sock_client );
                        log_msg( LOG_INFO, "Connection closed. Waiting for new client." );
                        break;
                }
            }
            // request for quit
            if ( !strncasecmp( l_buf, "quit", strlen( STR_QUIT ) ) )
            {
                close( l_sock_listen );
                close( l_sock_client );
                log_msg( LOG_INFO, "Request to 'quit' entered" );
                exit( 0 );
            }
        } // while communication

    G_SOCKETS[index][0] = -1;
    G_SOCKETS[index][1] = -1;

    pthread_exit(nullptr);
}

int get_available_index()
{
    for(int i = 0; i < MAX_CLIENTS; i++)
        if(G_SOCKETS[i][0] == -1)
            return i;

    return -1;
}

int main( int t_narg, char **t_args )
{
    int thread_param[ MAX_CLIENTS ][ 2 ];   // threads parameters
    pthread_t thread_id[ MAX_CLIENTS ];     // threads identification
    void* thread_status[ MAX_CLIENTS ];     // return values from threads

    int l_port = atoi(t_args[1]);

    log_msg( LOG_INFO, "Server will listen on port: %d.", l_port );

    // socket creation
    int l_sock_listen = socket( AF_INET, SOCK_STREAM, 0 );
    if ( l_sock_listen == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket.");
        exit( 1 );
    }

    in_addr l_addr_any = { INADDR_ANY };
    sockaddr_in l_srv_addr;
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_port = htons( l_port );
    l_srv_addr.sin_addr = l_addr_any;

    // Enable the port number reusing
    int l_opt = 1;
    if ( setsockopt( l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof( l_opt ) ) < 0 )
      log_msg( LOG_ERROR, "Unable to set socket option!" );

    // assign port number to socket
    if ( bind( l_sock_listen, (const sockaddr * ) &l_srv_addr, sizeof( l_srv_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Bind failed!" );
        close( l_sock_listen );
        exit( 1 );
    }

    // listenig on set port
    if ( listen( l_sock_listen, 1 ) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to listen on given port!" );
        close( l_sock_listen );
        exit( 1 );
    }

    log_msg( LOG_INFO, "Enter 'quit' to quit server." );

    // go!
    while ( 1 )
    {
        int l_sock_client = -1;

        // list of fd sources
        pollfd l_read_poll[ 2 ];

        l_read_poll[ 0 ].fd = STDIN_FILENO;
        l_read_poll[ 0 ].events = POLLIN;
        l_read_poll[ 1 ].fd = l_sock_listen;
        l_read_poll[ 1 ].events = POLLIN;

        while ( 1 ) // wait for new client
        {
            // select from fds
            int l_poll = poll( l_read_poll, 2, -1 );

            if ( l_poll < 0 )
            {
                log_msg( LOG_ERROR, "Function poll failed!" );
                exit( 1 );
            }
            
            if ( l_read_poll[ 1 ].revents & POLLIN && get_available_index() != -1)
            { // new client?
                sockaddr_in l_rsa;
                int l_rsa_size = sizeof( l_rsa );
                // new connection
                l_sock_client = accept( l_sock_listen, ( sockaddr * ) &l_rsa, ( socklen_t * ) &l_rsa_size );
                if ( l_sock_client == -1 )
                {
                        log_msg( LOG_ERROR, "Unable to accept new client." );
                        close( l_sock_listen );
                        exit( 1 );
                }
                uint l_lsa = sizeof( l_srv_addr );
                // my IP
                getsockname( l_sock_client, ( sockaddr * ) &l_srv_addr, &l_lsa );
                log_msg( LOG_INFO, "My IP: '%s'  port: %d",
                                 inet_ntoa( l_srv_addr.sin_addr ), ntohs( l_srv_addr.sin_port ) );
                // client IP
                getpeername( l_sock_client, ( sockaddr * ) &l_srv_addr, &l_lsa );
                log_msg( LOG_INFO, "Client IP: '%s'  port: %d",
                                 inet_ntoa( l_srv_addr.sin_addr ), ntohs( l_srv_addr.sin_port ) );

                break;
            }

        } // while wait for client

        int available_index = get_available_index();

        // parameters for thread
        G_SOCKETS[available_index][0] = l_sock_client;
        G_SOCKETS[available_index][1] = l_sock_listen;
        
        // create new thread
        int err = pthread_create(&thread_id[available_index], nullptr, handle_client, (void*)available_index);
        if ( err )
            log_msg( LOG_INFO, "Unable to create thread.");
        else
            log_msg( LOG_DEBUG, "Thread created.");

        // wait for thread
        //pthread_join( thread_id[G_CURRENT_THREAD_INDEX], nullptr);

    } // while ( 1 )

    return 0;
}
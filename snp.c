

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MAX_TIME_BETWEEN_MESSAGES	5
#define NUM_CLIENT			5

#define HOST_ID		8000
#define LAST_MESSAGE	(-1)

struct message
{
    int src_id;
    int src_pid;
    int data;
};

void fatal_error( char *s );
void server( int num_client, int in_socket, int out_socket[] );
void client( int number, int in_socket, int out_socket );


int main( void )
{
    int child_pid;
    int sock[NUM_CLIENT];
    int s[2];
    int server_socket[2];
    int i;

   
    if ( socketpair( AF_UNIX, SOCK_STREAM, PF_UNSPEC, server_socket ) < 0 )
	fatal_error( "socketpair (server_socket)" );

    for ( i = 0; i < NUM_CLIENT; i++ )
    {

	if ( socketpair( AF_UNIX, SOCK_STREAM, PF_UNSPEC, s ) < 0 )
	    fatal_error( "socketpair" );
	sock[i] = s[0];


	if ( ( child_pid = fork() ) < 0 )
	    fatal_error( "fork" );


	if ( child_pid == 0 )
	{
	    client( i, s[1], server_socket[1] );
	    exit( 0 );
	}
    }

 
    server( NUM_CLIENT, server_socket[0], sock );

    return 0;
}


void fatal_error( char *s )
{
    perror( s );
    exit( 1 );
}


void server( int num_client, int in_socket, int out_socket[] )
{
    struct message msg;
    int dest_id;
    int pid;

    pid = getpid();

    while ( num_client != 0 )
    {

	if ( recv( in_socket, &msg, sizeof msg, 0 ) != sizeof msg )
	    fatal_error( "server receive" );
	dest_id = msg.src_id;

	printf( "Process %2d:    ", msg.src_id );
	printf( "PID = %6d:    ", msg.src_pid );
	printf( "Message number %2d", msg.data );
	if ( msg.data == LAST_MESSAGE )
	{
	    num_client--;
	    printf( "    (clients remaining: %2d)", num_client );
	}
	printf( "\n" );


	msg.src_id = HOST_ID;
	msg.src_pid = pid;
	msg.data = 0;
	if ( send( out_socket[dest_id], &msg, sizeof msg, 0 ) != sizeof msg )
	    fatal_error( "server send" );
    }
}


void client( int number, int in_socket, int out_socket )
{
    int i;
    int pid;
    struct message msg;

    
    pid = getpid();
    srandom( 1000 * number + 10 * pid + 12345 );

 
    for ( i = 0; i < 5; i++)
    {

	sleep( random() % MAX_TIME_BETWEEN_MESSAGES );


	msg.src_id = number;
	msg.src_pid = pid;
	msg.data = i;

	if ( send( out_socket, &msg, sizeof msg, 0 ) != sizeof msg )
	    fatal_error( "client send 1" );
	if ( recv( in_socket, &msg, sizeof msg, 0 ) != sizeof msg )
	    fatal_error( "client receive 1" );
    }


    msg.src_id = number;
    msg.src_pid = pid;
    msg.data = LAST_MESSAGE;
    
    if ( send( out_socket, &msg, sizeof msg, 0 ) != sizeof msg )
	fatal_error( "client send 2" );
    if ( recv( in_socket, &msg, sizeof msg, 0 ) != sizeof msg )
	fatal_error( "client receive 2" );
}

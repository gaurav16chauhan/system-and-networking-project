* This program demonstrates how to use sockets (created with socketpair(2))
 * to perform message passing between a server process and several client
 * processes.
 *
 */

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

/*--------------------------------------------------------------------------*/

int main( void )
{
    int child_pid;
    int sock[NUM_CLIENT];
    int s[2];
    int server_socket[2];
    int i;

    /*
     * Create a pair of sockets and save one of them for the "server".
     */
    if ( socketpair( AF_UNIX, SOCK_STREAM, PF_UNSPEC, server_socket ) < 0 )
	fatal_error( "socketpair (server_socket)" );

    /*
     * Create a pair of sockets for each child process, then fork to create
     * the children.
     */
    for ( i = 0; i < NUM_CLIENT; i++ )
    {
	/*
	 * Create a pair of sockets and save one of them for the "server".
	 */
	if ( socketpair( AF_UNIX, SOCK_STREAM, PF_UNSPEC, s ) < 0 )
	    fatal_error( "socketpair" );
	sock[i] = s[0];

	/*
	 * Create a child...
	 */
	if ( ( child_pid = fork() ) < 0 )
	    fatal_error( "fork" );

	/*
	 * If we are a child process then be a "client".
	 */
	if ( child_pid == 0 )
	{
	    client( i, s[1], server_socket[1] );
	    exit( 0 );
	}
    }

    /*
     * Done creating children.  If we get here then we are the parent
     * and so we startup the server.
     */
    server( NUM_CLIENT, server_socket[0], sock );

    return 0;
}

/*--------------------------------------------------------------------------*/

void fatal_error( char *s )
{
    perror( s );
    exit( 1 );
}

/*--------------------------------------------------------------------------*/

void server( int num_client, int in_socket, int out_socket[] )
{
    struct message msg;
    int dest_id;
    int pid;

    pid = getpid();
    
    /*
     * Loop until there are no more clients.  Clients signal that they
     * are done by sending a message in which the data is LAST_MESSAGE.
     * When the server receives such a message, it reduces the value of
     * num_clients by one.
     */
    while ( num_client != 0 )
    {
	/*
	 * Receive a message from a client, and record who sent it so that
	 * we can reply to them.
	 */
	if ( recv( in_socket, &msg, sizeof msg, 0 ) != sizeof msg )
	    fatal_error( "server receive" );
	dest_id = msg.src_id;
	
	/*
	 * Display the message information.  Check for the data that
	 * signals that the client is done using the server.
	 */
	printf( "Process %2d:    ", msg.src_id );
	printf( "PID = %6d:    ", msg.src_pid );
	printf( "Message number %2d", msg.data );
	if ( msg.data == LAST_MESSAGE )
	{
	    num_client--;
	    printf( "    (clients remaining: %2d)", num_client );
	}
	printf( "\n" );

	/*
	 * Construct and send acknowledgement message
	 */
	msg.src_id = HOST_ID;
	msg.src_pid = pid;
	msg.data = 0;
	if ( send( out_socket[dest_id], &msg, sizeof msg, 0 ) != sizeof msg )
	    fatal_error( "server send" );
    }
}


/*--------------------------------------------------------------------------*/

void client( int number, int in_socket, int out_socket )
{
    int i;
    int pid;
    struct message msg;

    /*
     * Get my pid and initialize the random number generator
     */
    pid = getpid();
    srandom( 1000 * number + 10 * pid + 12345 );

    /*
     * Loop for the specified amount of times: compose and send messages
     * to the server process.
     */
    for ( i = 0; i < 5; i++)
    {
	/*
	 * Wait for a random number (between 0 and MAX_TIME_BETWEEN_MESSAGES-1
	 * inclusive) of seconds.
	 */
	sleep( random() % MAX_TIME_BETWEEN_MESSAGES );

	/*
	 * Construct the message
	 */
	msg.src_id = number;
	msg.src_pid = pid;
	msg.data = i;
	
	/*
	 * Send the message and wait for acknowledgement from server
	 */
	if ( send( out_socket, &msg, sizeof msg, 0 ) != sizeof msg )
	    fatal_error( "client send 1" );
	if ( recv( in_socket, &msg, sizeof msg, 0 ) != sizeof msg )
	    fatal_error( "client receive 1" );
    }

    /*
     * Done sending messages, send a message with a data value of
     * LAST_MESSAGE to signal that we are done.  After message is sent, wait
     * for an acknowledgement.
     */
    msg.src_id = number;
    msg.src_pid = pid;
    msg.data = LAST_MESSAGE;
    
    if ( send( out_socket, &msg, sizeof msg, 0 ) != sizeof msg )
	fatal_error( "client send 2" );
    if ( recv( in_socket, &msg, sizeof msg, 0 ) != sizeof msg )
	fatal_error( "client receive 2" );
}
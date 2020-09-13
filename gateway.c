#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <time.h>
#include "socket.h"  
#include "message.h"
#include "controller.h"

#define MAXFD(x,y) ((x) >= (y)) ? (x) : (y)

int main(int argc, char *argv[]){

	int port;
	struct cignal cig;

	// A buffer to store a serialized message
	char *cig_serialized = malloc(sizeof(char)*CIGLEN);

	// An array to registered sensor devices
	int device_record[MAXDEV] = {0};
	
	if(argc == 2){
		port = strtol(argv[1], NULL, 0);
	} else{
		fprintf(stderr, "Usage: %s port\n", argv[0]);
		exit(1);
	}

	int gatewayfd = set_up_server_socket(port);
	printf("\nThe Cignal Gateway is now started on port: %d\n\n", port);
	int peerfd;
	
	/* Explanation: Using select so that the server process never blocks on any call except
	 * select.
	 * 
	 * The server will handle connections from devices, will read a message from
	 * a sensor, process the message, write back a response message to the sensor client, 
	 * and close the connection. After reading a message it shows the serialized message 
	 * received from the client.
	 */
	
	// set up fd_set
	int max_fd = gatewayfd;
	fd_set all_fds;
	FD_ZERO(&all_fds);
	FD_SET(gatewayfd, &all_fds);

	// set up timeval
	struct timeval mytime;
	
	while(1) {
		// select with timer
		fd_set listen_fds = all_fds;
		mytime.tv_sec = 5;
		mytime.tv_usec = 0;
		int nready = select(max_fd + 1, &listen_fds, NULL, NULL, &mytime);

		if (nready == -1) {
			perror("server: select");
			exit(1);			
		} else if (nready == 0) {
			// if we returned from select with 0 fd ready then timer ran outp
			printf("Waiting for Sensors update...\n");
		} else {
			// if gatewayfd is still in listen_fds then set up connection (client requests connection)
			if (FD_ISSET(gatewayfd, &listen_fds)){
				peerfd = accept_connection(gatewayfd);
				if (peerfd > max_fd) max_fd = peerfd;
				FD_SET(peerfd, &all_fds);
				FD_CLR(gatewayfd, &listen_fds); // remove gatewayfd from listen_fds so it doesn't interfer with loop below
			}
			
			// for fd other than gatewayfd -- use some controller function (client sends message)
				// process the message properly
				// close the connection	
			for (int i = 0; i <= max_fd; i++) {
				if (FD_ISSET(i, &listen_fds)) {

					// get info from client
					if (read(i, cig_serialized, CIGLEN) != CIGLEN) {
						perror("Read failed\n");
						exit(1);				
					}

					unpack_cignal(cig_serialized, &cig);
					printf("RAW MESSAGE: %s\n", cig_serialized);
					
					// process the msg from client
					if (process_message(&cig, device_record) == 0) {
						// write back to client
						cig_serialized = serialize_cignal(cig);	
										
						if (write(i, cig_serialized, CIGLEN) != CIGLEN) {
							perror("Write failed\n");
							exit(1);				
						}
					}
					
					// now that we've written back (or if message isn't proper) we can close the connection
					FD_CLR(i, &all_fds);
					close(i);
				}
			}
			
			
		}
	}
	return 0;
}

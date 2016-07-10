#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUF_LEN 512

main()
{
	int sock, sock2;	// socket descripter
	int count;
	int datalen;
	in_port_t myport;	// server
	struct sockaddr_in myskt;	// socket address of the server
	struct sockaddr_in skt;	// socket address of the client
	char lbuf[BUF_LEN];
	int sktlen = sizeof(skt);
	// struct in_addr ipaddr;

	myport = 49152;	// port number

	// make a socket (TCP)
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("socket");
			exit(1);
	}

	// socket setting
	memset(&myskt, 0, sizeof(myskt)); // 0 clear
	myskt.sin_family = AF_INET;	// IPv4
	myskt.sin_port = htons(myport);	// port number
	myskt.sin_addr.s_addr = htonl(INADDR_ANY);	// any connection is ok


	// set port number to the socket
	if (bind(sock, (struct sockaddr *)&myskt, sizeof(myskt)) < 0) {
		perror("bind");
		exit(1);
	}

	// prepare for connection request
	if (listen(sock, 5) < 0) {
		perror("listen");
		exit(1);
	}

wait_conn_req:
	printf("waiting for connection ...\n");

	// accept connection request
	if ((sock2 = accept(sock, (struct sockaddr *)&skt, &sktlen)) < 0) {
		perror("accept");
		exit(1);
	}

	// print IP address and port number of the client
	printf("client\n\tIP address: %s, port number: %d\n",
			inet_ntoa(skt.sin_addr), ntohs(skt.sin_port));

	for (;;) {
		// receive data
		if (recv(sock2, lbuf, sizeof(lbuf), 0) < 0) {
			perror("recv");
			exit(1);
		}

		printf("received message: %s\n", lbuf);

		// send received data to the client
		if (send(sock2, lbuf, sizeof(lbuf), 0) < 0) {
			perror("send");
			exit(1);
		}

		if (strcmp(lbuf, "FIN") == 0) {
			printf("close socket\n");
			close(sock2);
			goto wait_conn_req;
		}
	}

	close(sock);

	return 0;
}


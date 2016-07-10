#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUF_LEN 512

main()
{
	int sock;	// socket descripter
	int count;
	int datalen;
	struct addrinfo hints, *res;
	struct sockaddr_in skt;	// socket address of the server
	char sbuf[BUF_LEN], lbuf[BUF_LEN], host[BUF_LEN];
	char *serv;
	int sktlen = sizeof(skt);
	int err;
	// struct in_addr ipaddr;

	serv = "49152";	// port number

	// input IP address of the server
	fprintf(stderr, "input hostname of the server: ");
	if (fgets(host, sizeof(host), stdin) == NULL) {
		if (feof(stdin) != 0) {
			return 0;
		} else {
			fprintf(stderr, "fgets() failed\n");
		}
	} else {
		// delete '\n'
		host[strlen(host) - 1] = '\0';
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	if ((err = getaddrinfo(host, serv, &hints, &res)) < 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		exit(1);
	}

	// make a socket (TCP)
	if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
		perror("socket");
		exit(1);
	}

	// connection request
	if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
		perror("connect");
		exit(1);
	}

	for (;;) {
		// input sending data
		fprintf(stderr, "input sending data: ");
		if (fgets(sbuf, sizeof(sbuf), stdin) == NULL) {
			strncpy(sbuf, "FIN", BUF_LEN);
		} else {
			// delete '\n'
			sbuf[strlen(sbuf) - 1] = '\0';
		}

		// send data to the server
		if (send(sock, sbuf, sizeof(sbuf), 0) < 0) {
			perror("send");
			exit(1);
		}

		// recerive data from the server
		if (recv(sock, lbuf, sizeof(lbuf), 0) < 0) {
			perror("recv");
			exit(1);
		}

		printf("received message: %s\n", lbuf);

		if (strcmp(lbuf, "FIN") == 0) {
			close(sock);
			printf("close socket\n");
			break;
		}
	}

	freeaddrinfo(res);
	return 0;
}


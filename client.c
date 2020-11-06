#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret, fdmax;
	struct sockaddr_in serv_addr;
	char buffer[sizeof(msgClient)];
	char command[30];
	char topic[51];
	char SF;


	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar

	if (argc != 4) {
		usage(argv[0]);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	int flag = 1;
	int result = setsockopt(sockfd,            /* socket affected */
                        IPPROTO_TCP,     /* set option at TCP level */
                        TCP_NODELAY,     /* name of option */
                        (char *) &flag,  /* the cast is historical cruft */
                        sizeof(int));    /* length of option value */
	DIE(result < 0, "TCP_NODELAY");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	FD_SET(sockfd, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
	if (sockfd > STDIN_FILENO){
		fdmax = sockfd;
	}
	else fdmax = STDIN_FILENO;

	msgClient *msg = calloc (1, sizeof(msgClient));
	strncpy(msg->id, argv[1], 10);

	n = send(sockfd, msg, sizeof(msgClient), 0);

	while (1)  {
		tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
			// se citeste de la tastatura
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN, stdin);

			if (strncmp(buffer, "exit", 4) == 0) {
				close(sockfd);
				return 1;

			} else if (strncmp(buffer, "subscribe", 9) == 0) {
				msg->type = 1;
				memset(msg->topic, 0, 50);

				sscanf(buffer, "%s %s %hhu", command, topic, &SF);
				strncpy(msg->topic, topic, 50);
				msg->SF = SF;

				n = send(sockfd, msg, sizeof(msgClient), 0);
				DIE(n < 0, "send");

				printf("subscribed %s\n", topic);

			} else if (strncmp(buffer, "unsubscribe", 11) == 0) {
				msg->type = 2;

				sscanf(buffer, "%s %s", command, topic);
				strncpy(msg->topic, topic, 50);

				n = send(sockfd, msg, sizeof(msgClient), 0);
				DIE(n < 0, "send");

				printf("unsubscribed %s\n", topic);

			} else {
				printf("undefined command [%s]\n", buffer);

			}
		}

		if (FD_ISSET(sockfd, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			n = recv(sockfd, buffer, sizeof(msgServ), 0);
			DIE(n < 0, "recv");
		

			if (n == 0) {
				/* server closed connection */
				return 0;

			} else if (n == sizeof(msgServ)) {
				msgServ* mesaj = (msgServ*) buffer;
				struct in_addr ip;
				ip.s_addr = mesaj->fromIP;

				memset(topic, 0, 51);
				strncpy(topic, mesaj->message, 51);

				printf("%s:%u - ", inet_ntoa(ip), ntohs(mesaj->fromPort));
				printf("%s - ", topic);

				unsigned short value;
				unsigned int valueI;
				unsigned char exp;
				unsigned int mod;

				switch (mesaj->message[50]) {
					case 0:
						printf("INT - ");

						if (mesaj->message[51] == 1)
							printf("-");

						valueI = * (unsigned int*) (&mesaj->message[52]);
						valueI = ntohl(valueI);

						printf("%d\n", valueI);

						break;

					case 1:
						printf("SHORT_REAL - ");

						value = * (unsigned short*) (&mesaj->message[51]);
						value = ntohs(value);

						printf("%d.%d\n", value/100, value%1000);

						break;

					case 2:
						printf("FLOAT - ");

						if (mesaj->message[51] == 1)
							printf("-");

						valueI = * (unsigned int*) (&mesaj->message[52]);
						valueI = ntohl(valueI);

						exp = mesaj->message[56];

						mod = pow(10, exp);

						if (mod < exp) {
							printf("0.");

							mod /= 10;

							do {
								mod /= 10;
								printf("0");
							} while(mod > valueI);

							printf("%d\n", valueI);

						} else {
							printf("%d.%d\n", valueI/mod, valueI%mod);

						}

						break;

					case 3:
						printf("STRING - %s\n", &mesaj->message[51]);

						break;

					default:
						printf("undefined data type\n"); 
						return 0;

				}

			} 
		}
	}

	close(sockfd);

	return 0;
}


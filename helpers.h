#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(EXIT_FAILURE);				\
		}							\
	} while(0)


#define BUFLEN 1551
#define MAX_CLIENTS 100
#define MAX_SUBS 100

#endif

typedef struct {
	char id[11];
	char type;
	char SF;
	char topic[51];
} msgClient;

typedef struct {
	unsigned long fromIP;
	unsigned short fromPort;
	char message[BUFLEN];
} msgServ;

typedef struct {
	char name[51];
	unsigned char SF;
}Subscriptions;


typedef struct client{
	int socket;
	char id_client[11];
	Subscriptions *abonamente;
	int index;
}fisa_client;


typedef struct{
	char **id_client;
	char topic_name[51];
	int *socketi;
	int index;
	int poz;
	int *SF;
}listTop;

typedef struct{
	msgServ mess;
	int socket;
	int index;
	char id_client[11];
}store;

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "helpers.h"


void usage(char *file){
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}




void prelucrareRegistru(msgClient *msg, fisa_client *registru, int decon,
						struct sockaddr_in* cli_addr, int sock,
				  		fd_set* read_fds, int *nrMemb, listTop *top, store *st){


	int i;
	struct in_addr ip   = cli_addr->sin_addr;
	unsigned short port = cli_addr->sin_port;

	if (decon == 0){
		/* clientul s-a deconectat */
		for (i = 0; i < MAX_CLIENTS; i++){
			// vad ce client s-a deconectat
			if (registru[i].socket == sock){
				printf("Client %s disconnected.\n", registru[i].id_client);
				
				close(registru[i].socket);
				break;
			}
		}
		FD_CLR(sock, read_fds);
	}
	else{
		/* cazul in care clientul se reconecteaza sau vrea sa
		faca o prelucrare pe topicuri */
		int poz = -1;
		for (i = 0; i < *nrMemb; i++){
			if (strcmp(registru[i].id_client, msg->id) == 0){
				/* am gasit de la ce client am primit mesajul */
				poz = i;
				break;
			}
		}
		if (poz != -1){
			Subscriptions newSub;
			
			if (msg->type == 0){
				/*clientul s-a reconectat ; ii salvez socketul*/
				registru[poz].socket = sock;
				printf("Client %s reconnected.\n", msg->id);
				/*retrimitere mesaje precedente */
				int count = 0;
				for (int i = 0; i < st->index; i++){
					if (strcmp(msg->id, st[i].id_client) == 0 || sock == st[i].socket){
						int n = send(sock, &st[i].mess, sizeof(msgServ), 0);
						count++;
						/* o sa imi suprascrie mesajele din store
							nu mai e nevoie sa le sterg*/
					}
				}
				st->index -= count;
			}

			if (msg->type == 1){
				memset(&newSub, 0, sizeof(Subscriptions));

				strncpy(newSub.name, msg->topic, 50);
				newSub.SF = msg->SF;
				memcpy(&registru[poz].abonamente[registru[poz].index], &newSub, sizeof(Subscriptions));
				registru[poz].index ++;

				printf("Client %s subscribed %s\n", msg->id, msg->topic);
				int found = 0;
				for (int k = 0; k < top->index; k++){
					if (strcmp(top[k].topic_name, msg->topic) == 0){

						// memcpy(&top[k].id_client[top->poz], msg->id, 11);
						strcpy(top[k].id_client[top->poz], msg->id);
						top[k].socketi[top->poz] = sock;
						top[k].SF[top->poz] = msg->SF;
						top->poz ++;
						found = 1;
						break;
					}
				}
				if (found == 0){
					/* daca nu am gasit nici un topic existent cu acest nume
					il creez eu */
					strncpy(top[top->index].topic_name, msg->topic, 50);
					top[top->index].socketi = calloc(10, sizeof(int));
					top[top->index].id_client = calloc(10, sizeof(char*));
					for (int i = 0; i < 10; i++){
						top[top->index].id_client[i] = calloc(11, sizeof(char));
					}
					top[top->index].SF = calloc(10, sizeof(int));

					/* adaug in structura de topicuri elementul nou creat */
					strcpy(top[top->index].id_client[top->poz], msg->id);
					top[top->index].socketi[top->poz] = sock;
					top[top->index].SF[top->poz] = msg->SF;
					top->poz ++;
					found = 1;

					top->index ++;
				}
				
			}
			else if (msg->type == 2){
				// unsubscribe
				printf("Client %s unsubscribed topic : %s\n", msg->id, msg->topic);


				for (int k = 0; k < top->index; k++){
					/*verific topicul */
					if (strcmp(top[k].topic_name, msg->topic) == 0){
						// daca corespunde , verific vectorul de clienti abonati
						for (int x = 0; x < top->poz; x++){
							if (strcmp(top[k].id_client[x], msg->id) == 0){
								/*sterg clientul ce nu mai vrea abonament la
								acest topic*/
								top[k].id_client[x] = 0;
								top[k].socketi[x] = 0;
								for (int y = x; y < top->poz - 1; y++){
									memcpy(&top[k].id_client[y], &top[k].id_client[y + 1], sizeof(char) * 11);
									memcpy(&top[k].socketi[y], &top[k].socketi[y + 1], sizeof(int));
									/* rearanjez in memorie elementele din vector */
								}
								top[k].poz--;
								break;
							}
						}
						break;
					}
				}
			}
		}
		else {
			/* clientul trebuie adaugat la lista */
			printf("New client %s connected from %s:%u.\n", msg->id, inet_ntoa(ip), ntohs(port));

			memcpy(&registru[*nrMemb].id_client, msg->id, sizeof(msg->id));
			registru[*nrMemb].socket = sock;
			registru[*nrMemb].index = 0;
			registru[*nrMemb].abonamente = calloc(MAX_SUBS, sizeof(char) * 51);

			
			(*nrMemb)++;
		}
	}

}


void sendNews(msgServ msg, listTop *top, fd_set read_fds, char *topicName, store *st){

	fd_set tmp_fds = read_fds;
	for (int i = 0; i < top->index; i++){
		/* parcurg topicurile */
		if (strcmp(top[i].topic_name, topicName) == 0){
			/* verific daca topicul corespunde cu cel ce trebuie trimis */
			int count = 0;
			for (int j = 0; j < top->poz; j ++){
				/* trimit la toti utilizatorii conectati */
				if (FD_ISSET(top[i].socketi[j], &tmp_fds)){
					/* verific ce utilizator este in momentul acesta conectat */
					int n = send(top[i].socketi[j], &msg, sizeof(msgServ), 0);
					count ++;
					
				}
				else{
					/*clientul s-a deconectat, deci adaugam mesajul in store 
					  pentru a putea sa-l forwardam atunci cand se reconecteaza*/
					if (top[i].SF[j] == 1){
						strcpy(st[st->index].id_client, top[i].id_client[j]);
						memcpy(&st[st->index].mess, &msg, sizeof(msgServ));			
						st[st->index].socket = top[i].socketi[j];
						st->index++;	
					}
				}
			}
			return;
		}
	}
	

}

void UDP(char *mesaj, listTop *top, int dim, 
		struct sockaddr_in* cli_addr, 
		fd_set read_fds, int sock, store *st){
	msgServ msg;

	char topicName[51];
	memset(topicName, 0, 51);
	strncpy (topicName, mesaj, 50);

	/* caut sa vad daca am creat deja topicul in lista de topicuri */
	int found = 0;
	for (int i = 0; i < top->index; i++){
		if (strcmp(top[i].topic_name, topicName) == 0){
			found = 1;
			break;
		}
	}
	/* daca nu l-am gasit , atunci il creez si astept sa inserez clienti
	   vectorul destinat clientilor din structura */
	if (found == 0){
		strncpy(top[top->index].topic_name, topicName, 50);
		top[top->index].socketi = calloc(10, sizeof(int));
		top[top->index].id_client = calloc(10, sizeof(char*));
		for (int i = 0; i < 10; i++){
			top[top->index].id_client[i] = calloc(11, sizeof(char));
		}
		top[top->index].SF = calloc(10, sizeof(int));
		top->index ++;
		return;
	}

	memset(&msg, 0, sizeof(msgServ));
	/* pregatesc mesajul pentru trimitere */
	msg.fromIP = cli_addr->sin_addr.s_addr;
	msg.fromPort = cli_addr->sin_port;
	memcpy (msg.message, mesaj, dim);
	sendNews(msg, top, read_fds, topicName, st);
}

int main(int argc, char *argv[])
{	
	int tcpsock, updsock, newSock;
	char buffer[BUFLEN];
	
	fisa_client *registru = calloc(MAX_CLIENTS, sizeof(fisa_client));
	listTop *top = calloc(100, sizeof(listTop));
	store *st = calloc(100, sizeof(store));

	
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret , flag;
	socklen_t clilen;

	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;

	if (argc < 2) {
		usage(argv[0]);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	tcpsock = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcpsock < 0, "socket");
	
	ret = setsockopt(tcpsock, IPPROTO_TCP, TCP_NODELAY, (void *)&flag, sizeof(flag));
    DIE(tcpsock == 0, "TCP_NODELAY");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = INADDR_ANY;


	ret = bind(tcpsock, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(tcpsock, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(tcpsock, &read_fds);
	fdmax = tcpsock;

	updsock = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(updsock < 0, "socket");

	ret = bind(updsock, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	FD_SET(updsock, &read_fds);
	if (updsock > fdmax)
		fdmax = updsock;
	
	FD_SET(STDIN_FILENO, &read_fds);
	if (STDIN_FILENO > fdmax)
		fdmax = STDIN_FILENO;

	int nrMemb = 0;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == STDIN_FILENO) {
					/* input from user */
					
					memset(buffer, 0, BUFLEN);
					n = read(STDIN_FILENO, buffer, BUFLEN);
					if(strncmp(buffer, "exit\n", 4) == 0) {
						close(tcpsock);
						close(updsock);
						return 0;

					} else {
						printf("undefined input\n");
						return 0;

					} 
				}
				else if (i == tcpsock) {
					/* tcp client connection */
					clilen = sizeof(cli_addr);
					newSock = accept(tcpsock, (struct sockaddr*) &cli_addr, &clilen);
					DIE(newSock < 0, "accept");

					FD_SET(newSock, &read_fds);
					if (newSock > fdmax)
						fdmax = newSock;
					
					/* adaugare in registru de gestiune */


				} else if (i == updsock) {
					/* udp post on server */
					clilen = sizeof(cli_addr);
					
					n = recvfrom(updsock, buffer, BUFLEN, 0, (struct sockaddr*) &cli_addr, &clilen);
					DIE(n < 0, "recvfrom");

					/* udp message interpret*/
					UDP(buffer, top, BUFLEN, &cli_addr, read_fds, i, st);

				} else {
					
					/* message from tcp client */
					memset(buffer, 0, BUFLEN);

					n = recv(i, buffer, BUFLEN, 0);
					DIE(n < 0, "recv");
					msgClient *mess = (msgClient*)buffer;
					prelucrareRegistru(mess, registru, n, &cli_addr, i, &read_fds, &nrMemb, top, st);
					/* see what client needs */
					
				}
			}
		}
	}

	close(tcpsock);
	close(updsock);
	
	return 0;
}


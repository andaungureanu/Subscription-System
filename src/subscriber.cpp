#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include "helper.h"
#define BUFLEN 1600

using namespace std;

void usage(char *file) {
	fprintf(stderr, "Usage: %s <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n", file);
	exit(0);
}

int main(int argc, char* argv[]) {
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	char id[10];
	int flag = 1;

	fd_set read_fds;
    fd_set tmp_fds;
    int fdmax;
    FD_ZERO(&tmp_fds);
    FD_ZERO(&read_fds);

	if (argc < 4) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "Couldn't create socket");
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(int));

	FD_SET(sockfd, &read_fds);
    fdmax = sockfd;
    FD_SET(0, &read_fds);

	serv_addr.sin_family = AF_INET;
 	serv_addr.sin_port = htons(atoi(argv[3]));
 	ret = inet_aton(argv[2], &serv_addr.sin_addr);
 	DIE(ret == 0, "Couldn't use inet_aton properly");

 	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
 	DIE(ret < 0, "Couldn't connect to server");

 	DIE(strlen(argv[1]) > 10, "Your ID is too long, please choose another one");
 	sprintf(id, "%s", argv[1]);
 	n = send(sockfd, id, BUFLEN, 0);
 	DIE(n < 0, "Couldn't send ID");

 	while (1) {
 		tmp_fds = read_fds;
 		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
    	DIE(ret < 0, "Couldn't select");

    	if (FD_ISSET(0, &tmp_fds)) {  // comanda de la tastatura
    		memset(buffer, 0, BUFLEN);
      		fgets(buffer, BUFLEN - 1, stdin);
      		if (strncmp(buffer, "exit", 4) == 0) { // exit
      			if (strcmp(buffer, "exit\n") != 0) { // programare defensiva
      				fprintf(stderr, "You can only use exit.\n");
      			} else {
	      			n = send(sockfd, buffer, BUFLEN, 0);
	      			DIE(n < 0, "Couldn't send exit command");
					exit(0);
				}
			} else {
				if (strncmp(buffer, "subscribe", 9) == 0) { // comanda de subscribe
					// ------ programare defensiva ------
					char copy[BUFLEN];
					int ok = 1;
					memcpy(copy, buffer, BUFLEN);
					char* command = strtok(copy, " \n");
					if (strcmp(command, "subscribe") != 0) {
						fprintf(stderr, "Only subscribe works.\n");
						ok = 0;
					} else {
						char* topic = strtok(NULL, " \n");
						if (topic == NULL) {
							fprintf(stderr, "Not enough arguments for subscribe.\n");
							ok = 0;
						} else {
							if (strlen(topic) > 50) {
								fprintf(stderr, "Topic shouldn't have more than 50 characters.\n");
								ok = 0;
							}
							char* sf = strtok(NULL, " \n");
							if (sf == NULL) {
								fprintf(stderr, "Not enough arguments for subscribe.\n");
								ok = 0;
							} else {
								if (atoi(sf) != 1 && atoi(sf) != 0) {
									fprintf(stderr, "SF can only be 1 or 0.\n");
									ok = 0;
								}
								char* end_of_command = strtok(NULL, " \n");
								if (end_of_command != NULL) {
									fprintf(stderr, "Too many arguments for subscribe.\n");
									ok = 0;
								}
							}
						}
					}
					// ----------------------------------

					if (ok == 1) {
						n = send(sockfd, buffer, BUFLEN, 0);
						DIE(n < 0, "Couldn't send subscribe request");

						memset(buffer, 0, BUFLEN);
						n = recv(sockfd, buffer, BUFLEN, 0);
						DIE(n < 0, "Couldn't receive response from server");
						if (strncmp(buffer, "success", 7) == 0) {
							printf("Subscribed to topic.\n");
							fflush(stdout);
						}
					}
				} else if (strncmp(buffer, "unsubscribe", 11) == 0) { // comanda de unsubscribe
					// ------ programare defensiva ------
					char copy[BUFLEN];
					int ok = 1;
					memcpy(copy, buffer, BUFLEN);
					char* command = strtok(copy, " \n");
					if (strcmp(command, "unsubscribe") != 0) {
						fprintf(stderr, "Only unsubscribe works.\n");
						ok = 0;
					} else {
						char* topic = strtok(NULL, " \n");
						if (topic == NULL) {
							fprintf(stderr, "Not enough arguments for unsubscribe.\n");
							ok = 0;
						} else {
							if (strlen(topic) > 50) {
								fprintf(stderr, "Topic shouldn't have more than 50 characters.\n");
								ok = 0;
							}
							char* end_of_command = strtok(NULL, " \n");
							if (end_of_command != NULL) {
								fprintf(stderr, "Too many arguments for unsubscribe.\n");
								ok = 0;
							}
						}
					}
					// ----------------------------------

					if (ok == 1) {
						n = send(sockfd, buffer, BUFLEN, 0);
						DIE(n < 0, "Couldn't send unsubscribe request");

						memset(buffer, 0, BUFLEN);
						n = recv(sockfd, buffer, BUFLEN, 0);
						DIE(n < 0, "Couldn't receive response from server");
						if (strncmp(buffer, "success", 7) == 0) {
							printf("Unsubscribed from topic.\n");
							fflush(stdout);
						}
					}
				}	
			}	
		} else if (FD_ISSET(sockfd, &tmp_fds)) { // primeste ceva de la server
			char received[1600];
			n = recv(sockfd, received, BUFLEN, 0);
	    	DIE(n < 0, "Couldn't receive message from server");
	    	if (strcmp(received, "ID problem") == 0) {
	    		break;
	    	} else if (strncmp(received, "exit", 4) == 0) {
	    		break;
	    	}
	    	printf("%s\n", received); // afiseaza mesajul de la topicul la care e abonat
	    	fflush(stdout);
		}
	}
	for (int i = 3; i <= fdmax; i++) {
		close(i);
	}
	return 0;	
}
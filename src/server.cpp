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
#include <queue>
#include <vector>
#include "helper.h"
#define BUFLEN 1600

using namespace std;

struct client {
	char id[10];
	int fd;
	int status;
	queue<string> incoming;
};

struct topic {
	char name[51];
	vector<client> sf1;
	vector<client> sf0;
};

int user_position(char* id, vector<client> v) {
	int result = -1;
	for (unsigned int i = 0; i < v.size(); i++) {
		if (strcmp(id, v[i].id) == 0) {
			result = i;
		}
	}
	return result;
}

int fd_position(int fd, vector<client> v) {
	int result = -1;
	for (unsigned int i = 0; i < v.size(); i++) {
		if (fd == v[i].fd) {
			result = i;
		}
	}
	return result;
}

int topic_position(char* name, vector<topic> v) {
	int result = -1;
	for (unsigned int i = 0; i < v.size(); i++) {
		if (strcmp(name, v[i].name) == 0) {
			result = i;
		}
	}
	return result;
}

void usage(char *file) {
	fprintf(stderr, "Usage: %s <PORT_DORIT>\n", file);
	exit(0);
}

int main(int argc, char* argv[]) {
	int tsockfd, usockfd, clisockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, ret;
	socklen_t clilen;
	int flag = 1;

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	if (argc < 2) {
		usage(argv[0]);
	}

	usockfd = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(usockfd < 0, "Couldn't create socket for udp");
	tsockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tsockfd < 0, "Couldn't create socket for tcp");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
 	serv_addr.sin_port = htons(atoi(argv[1]));
 	serv_addr.sin_addr.s_addr = INADDR_ANY;

 	ret = bind(usockfd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr));
  	DIE(ret < 0, "Couldn't bind udp socket");
  	ret = bind(tsockfd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr));
  	DIE(ret < 0, "Couldn't bind tcp socket");
  	ret = listen(tsockfd, 100);
  	DIE(ret < 0, "Couldn't listen");

  	FD_SET(tsockfd, &read_fds);
  	FD_SET(usockfd, &read_fds);
  	FD_SET(0, &read_fds);
  	if (usockfd > tsockfd) {
  		fdmax = usockfd;
  	} else {
  		fdmax = tsockfd;
  	}

  	portno = atoi(argv[1]);
  	vector<client> clients;
  	vector<topic> topics;

  	while (1) {
  		tmp_fds = read_fds;
  		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
  		DIE(ret < 0, "Couldn't select");
  		for (int i = 0; i <= fdmax; i++) {
  			if (FD_ISSET(i, &tmp_fds)) {
  				if (i == 0) { // primeste de la tastatura
  					memset(buffer, 0, BUFLEN);
  					fgets(buffer, BUFLEN - 1, stdin);
  					if (strncmp(buffer, "exit", 4) == 0) { // exit
  						if (strcmp(buffer, "exit\n") != 0) { // programare defensiva
  							fprintf(stderr, "Server can only use exit.\n");
  						} else {
	  						for (unsigned int j = 0; j < clients.size(); j++) {
	  							if (clients[j].status == 1) {
	  								n = send(clients[j].fd, buffer, BUFLEN, 0);
	  								DIE(n < 0, "Couldn't send update");
	  							}
	  						}
	  						exit(0);
	  					}
  					} else {
  						fprintf(stderr, "Server can only use exit.\n"); // programare defensiva
  					}	
  				} else if (i == usockfd) { // primeste de la clientul udp
  					memset(buffer, 0, BUFLEN);
  					clilen = sizeof(cli_addr);
  					n = recvfrom(usockfd, buffer, BUFLEN, 0, (struct sockaddr*) &serv_addr, &clilen);
  					DIE(n < 0, "Couldn't receive message from udp");

  					char copy[1600];
  					char topic_name[51];
  					memcpy(topic_name, buffer, 50);
  					topic_name[50] = '\0';
  					uint8_t data_type = buffer[50];
  					char* udp_ip = inet_ntoa(serv_addr.sin_addr);

  					// prelucrez payload-ul primit de la udp
  					if (data_type == 0) {
    					uint32_t payload;
    					memcpy(&payload, buffer + 52, sizeof(uint32_t));
    					payload = ntohl(payload);
    					if (buffer[51] == 1) {
    						sprintf(copy, "%s:%d - %s - INT - -%d", udp_ip, portno, topic_name, payload);
    					} else {
    						sprintf(copy, "%s:%d - %s - INT - %d", udp_ip, portno, topic_name, payload);
    					}
    				} else if (data_type == 1) {
    					uint16_t payload;
    					memcpy(&payload, buffer + 51, sizeof(uint16_t));
    					payload = ntohs(payload);
    					float new_payload = (float) payload / 100;
    					sprintf(copy, "%s:%d - %s - SHORT_REAL - %.2f", udp_ip, portno, topic_name, new_payload);
    				} else if (data_type == 2) {
    					uint32_t payload;
    					memcpy(&payload, buffer + 52, sizeof(uint32_t));
    					payload = ntohl(payload);
    					uint8_t exp = buffer[56];
    					uint8_t copy_exp = exp;
    					float new_payload = (float) payload;
    					while (copy_exp) {
    						new_payload /= 10;
    						copy_exp--;
    					}
    					if (buffer[51] == 1) {
    						sprintf(copy, "%s:%d - %s - FLOAT - -%.*f", udp_ip, portno, topic_name, exp, new_payload);
    					} else {
    						sprintf(copy, "%s:%d - %s - FLOAT - %.*f", udp_ip, portno, topic_name, exp, new_payload);
    					}
    				} else if (data_type == 3) {
    					sprintf(copy, "%s:%d - %s - STRING - %s", udp_ip, portno, topic_name, buffer + 51);
    				}

    				int pos = topic_position(topic_name, topics); // caut topic-ul sa vad daca exista abonati
    				if (pos != -1) { // daca exista abonati
    					int fd;
	    				for (unsigned int j = 0; j < topics[pos].sf0.size(); j++) {
	    					if (topics[pos].sf0[j].status == 1) { // trimit mesajul la cei cu sf 0 si online
	    						fd = topics[pos].sf0[j].fd;
	    						n = send(fd, copy, BUFLEN, 0);
	    						DIE(n < 0, "Couldn't send updates to client");
	    					}
	    				}
	    				for (unsigned int j = 0; j < topics[pos].sf1.size(); j++) { // trimit mesajul la cei cu sf1
	    					if (topics[pos].sf1[j].status == 1) { // daca e online il trimit direct
	    						fd = topics[pos].sf1[j].fd;
	    						n = send(fd, copy, BUFLEN, 0);
	    						DIE(n < 0, "Couldn't send updates to client");
	    					} else { // daca e offline ii salvez mesajul pentru cand se reconecteaza
	    						char* id = topics[pos].sf1[j].id;
	    						int pos2 = user_position(id, clients);
	    						clients[pos2].incoming.push(copy);
	    					}
	    				}
    				}
  				} else if (i == tsockfd) { // primeste o cerere de conexiune
  					clilen = sizeof(cli_addr);
  					clisockfd = accept(tsockfd, (struct sockaddr*) &cli_addr, &clilen);
  					DIE(clisockfd < 0, "Couldn't accept new connection");
  					setsockopt(clisockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(int));
  					memset(buffer, 0, BUFLEN);
  					n = recv(clisockfd, buffer, BUFLEN, 0); // id-ul clientului
  					if (user_position(buffer, clients) == -1) { // daca nu s-a mai conectat pana acum
  						client newu; // creez clientul nou
  						strcpy(newu.id, buffer);
  						newu.fd = clisockfd;
  						newu.status = 1;
  						clients.push_back(newu); // il adaug la lista de clienti conectati pana acum
  						FD_SET(clisockfd, &read_fds);
  						if (clisockfd > fdmax) {
  							fdmax = clisockfd;
  						}
  						printf("New client %s connected from %s:%d.\n", buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
  						fflush(stdout);
  					} else { // daca exista deja in lista de clienti
  						int pos = user_position(buffer, clients);
  						if (clients[pos].status == 1) { // daca e online, anunt noul client ca nu poate avea acelasi ID
  							printf("Client %s already connected.\n", clients[pos].id);
  							fflush(stdout);
  							n = send(clisockfd, "ID problem", BUFLEN, 0);
  							DIE(n < 0, "Couldn't contact client");
  							close(clisockfd);
  							break;
  						} else { // daca e offline atunci se reconecteaza
  							FD_SET(clisockfd, &read_fds);
  							if (clisockfd > fdmax) {
  								fdmax = clisockfd;
  							}
  							clients[pos].status = 1;
  							clients[pos].fd = clisockfd; // actualizez statusul si sockfd-ul in lista de clienti
  							printf("New client %s connected from %s:%d.\n", buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
  							fflush(stdout);

  							// actualizez statusul si sockfd-ul intr-una din listele cu sf1 sau sf0
  							// de la topic-urile la care era abonat
  							for (unsigned int j = 0; j < topics.size(); j++) {
	  							for (unsigned int k = 0; k < topics[j].sf0.size(); k++) {
	  								if (strcmp(topics[j].sf0[k].id, clients[pos].id) == 0) {
	  									topics[j].sf0[k].status = 1;
	  									topics[j].sf0[k].fd = clisockfd;
	  								}
	  							}
	  							for (unsigned int k = 0; k < topics[j].sf1.size(); k++) {
	  								if (strcmp(topics[j].sf1[k].id, clients[pos].id) == 0) {
	  									topics[j].sf1[k].status = 1;
	  									topics[j].sf1[k].fd = clisockfd;
	  								}
	  							}
	  						}
	  						// daca are mesaje in coada, atunci i le trimit pe rand si eliberez coada
  							while(!clients[pos].incoming.empty()) {
  								n = send(clisockfd, clients[pos].incoming.front().c_str(), BUFLEN, 0);
  								DIE(n < 0, "Couldn't send updates to client");
  								clients[pos].incoming.pop();
  							}
  							clients[pos].incoming = queue<string>();
  						}
  					}
  				} else { // exit/subscribe/unsubscribe
  					memset(buffer, 0, BUFLEN);
  					n = recv(i, buffer, BUFLEN, 0);
  					DIE(n < 0, "Couldn't receive command");
  					if (strncmp(buffer, "exit", 4) == 0) { // exit
  						int pos = fd_position(i, clients);
  						printf("Client %s disconnected.\n", clients[pos].id);
  						fflush(stdout);
  						clients[pos].status = 0;
  						clients[pos].fd = -1; // actualizez statusul si sockfd-ul in lista de clienti


  						// actualizez statusul si sockfd-ul intr-una din listele cu sf1 sau sf0
  						// de la topic-urile la care era abonat
  						for (unsigned int j = 0; j < topics.size(); j++) {
  							for (unsigned int k = 0; k < topics[j].sf0.size(); k++) {
  								if (strcmp(topics[j].sf0[k].id, clients[pos].id) == 0) {
  									topics[j].sf0[k].status = 0;
  									topics[j].sf0[k].fd = -1;
  								}
  							}
  						}
  						for (unsigned int j = 0; j < topics.size(); j++) {	
  							for (unsigned int k = 0; k < topics[j].sf1.size(); k++) {
  								if (strcmp(topics[j].sf1[k].id, clients[pos].id) == 0) {
  									topics[j].sf1[k].status = 0;
  									topics[j].sf1[k].fd = -1;
  								}
  							}
  						}
  						FD_CLR(i, &read_fds);
  						close(i);
  					} else { // mesaj de la client
  						char* command = strtok(buffer, " ");
  						if (strncmp(command, "subscribe", 9) == 0) {
  							char* topic_name = strtok(NULL, " ");
  							int pos = fd_position(i, clients);
  							char* sf = strtok(NULL, " ");
  							if (topic_position(topic_name, topics) == -1) { // nu exista topicul
  								topic newt; // creez topicul nou si ii completez campurile
  								strcpy(newt.name, topic_name);
  								if (atoi(sf) == 0) {
  									newt.sf0.push_back(clients[pos]);
  								} else if (atoi(sf) == 1) {
  									newt.sf1.push_back(clients[pos]);
  								}
  								n = send(clisockfd, "success", BUFLEN, 0);
  								DIE(n < 0, "Couldn't send feedback to client");
  								topics.push_back(newt); // il adaug in topics
  							} else { // exista topicul deja
  								int tpos = topic_position(topic_name, topics);
  								int upos1 = user_position(clients[pos].id, topics[tpos].sf1); 
                  				// pozitia clientului in lista de sf1 a topicului dorit
  								int upos0 = user_position(clients[pos].id, topics[tpos].sf0);
                  				// pozitia clientului in lista de sf0 a topicului dorit
  								if (upos1 == -1 && upos0 == -1) { // daca nu e in niciuna, il adaug dupa sf
  									if (atoi(sf) == 0) {
  										topics[tpos].sf0.push_back(clients[pos]);
  									} else {
  										topics[tpos].sf1.push_back(clients[pos]);
  									}
  									n = send(clisockfd, "success", BUFLEN, 0);
		    						DIE(n < 0, "Couldn't send feedback to client");
  								} else if (upos1 == -1 && upos0 != -1) { 
  									if (atoi(sf) == 1) { // e in sf0, vrea in sf1
  										topics[tpos].sf1.push_back(clients[pos]);
  										topics[tpos].sf0.erase(topics[tpos].sf0.begin() + upos0);
  										n = send(clisockfd, "success", BUFLEN, 0);
		    							DIE(n < 0, "Couldn't send feedback to client");
  									}
  								} else if (upos1 != -1 && upos0 == -1) {
  									if (atoi(sf) == 0) { // e in sf1, vrea in sf0
  										topics[tpos].sf0.push_back(clients[pos]);
  										topics[tpos].sf1.erase(topics[tpos].sf1.begin() + upos1);
  										n = send(clisockfd, "success", BUFLEN, 0);
		    							DIE(n < 0, "Couldn't send feedback to client");
  									}
  								} else {
  									n = send(clisockfd, "no changes", BUFLEN, 0);
  									DIE(n < 0, "Couldn't send feedback to client");
  								}
  						  	}
  						} else if (strncmp(command, "unsubscribe", 11) == 0) {
  							char* topic_name = strtok(NULL, "\n");
  							int tpos = topic_position(topic_name, topics);
  							int pos = fd_position(i, clients);
  							if (tpos != -1) {
	  							int upos1 = user_position(clients[pos].id, topics[tpos].sf1);
	  							int upos0 = user_position(clients[pos].id, topics[tpos].sf0);
	  							if (upos0 != -1) { // daca e in sf0 din topicul ales
	  								topics[tpos].sf0.erase(topics[tpos].sf0.begin() + upos0);
	  								n = send(clisockfd, "success", BUFLEN, 0);
			    					DIE(n < 0, "Couldn't send feedback to client");
	  							} else if (upos1 != -1) { // daca e in sf1 din topicul ales
	  								topics[tpos].sf1.erase(topics[tpos].sf1.begin() + upos1);
	  								n = send(clisockfd, "success", BUFLEN, 0);
			    					DIE(n < 0, "Couldn't send feedback to client");
	  							}
	  						} else {
	  							n = send(clisockfd, "no changes", BUFLEN, 0);
	  							DIE(n < 0, "Couldn't send feedback to client");
	  						}
  						}
  					}
  				}
  			}
  		}
  	}
  	for (int i = 3; i <= fdmax; i++) {
		close(i);
	}
  	return 0;
}




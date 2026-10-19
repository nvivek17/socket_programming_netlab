#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "arpa/inet.h"

#include <cassert>
#include <string>
#include <bitset>

#define MAXDATASIZE 1000
#define PORT 8087
using namespace std;
string conv_to_bin(string s){
    string bin;
    for(unsigned char c : s){
        bin+= bitset<8>(c).to_string();
    }
    return bin;
}

int main(int argc , char* argv[]){

    assert(argc == 4);


    int sockfd, numbytes;
    char buffer[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in their_addr;
    // if (argc != 2) {
    //     fprintf(stderr,"usage: client hostname\n");
    //     exit(1);
    // }
    // if ((he=gethostbyname(argv[1])) == NULL) {
    // perror("gethostbyname");
    // exit(1);
    // }
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    char hostname[100];
    gethostname(hostname,(size_t)(100));
    printf("text: %s",hostname);
    their_addr.sin_family = AF_INET; 
    their_addr.sin_port = htons(PORT); 
    // their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    inet_pton(AF_INET,argv[1] , &their_addr.sin_addr);

    bzero(&(their_addr.sin_zero), 8); 
    socklen_t sock_len=sizeof(struct sockaddr);
    if (connect(sockfd, (struct sockaddr *)&their_addr,sock_len) == -1) {
    perror("connect");
    exit(1);
    }
    
    string bin_msg = conv_to_bin(std::string(argv[3])+ "," + std::string(argv[2]));
    send(sockfd, bin_msg.data(), sizeof(bin_msg.data()),0);

    numbytes=recv(sockfd, buffer, MAXDATASIZE-1, 0);
    if ( numbytes== -1) {
    perror("recv");
    exit(1);
    }
    buffer[numbytes] = '\0';
    printf("Received: %s",buffer);
    close(sockfd);
}

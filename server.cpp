#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <bitset>

#include <string>
#include <vector>
#include <fstream>
#include <cassert>
#include <cctype>

using namespace std;

void parse_matrix(char* filepath, vector<vector<pair<int, int>>>& adj);

using namespace std;

#define PORT 8087
#define BACKLOG 26
#define MAXDATASIZE 100

string conv_to_bin(string s){
    string bin;
    for(unsigned char c : s){
        bin+= bitset<8>(c).to_string();
    }
    return bin;
}

int main(int argc, char * argv[]){

    assert(argc == 2);
    
    int socketfd;
    int ctr = 1;
    int other_fd[ctr]; 

    struct sockaddr_in my_addr;
    char buffer[MAXDATASIZE];
    struct sockaddr_in their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;

    if((socketfd=socket(AF_INET,SOCK_STREAM,0))==-1){
        perror("Socket not given");exit(1);
    }

    if (setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    my_addr.sin_port=htons(PORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_family=AF_INET;
    bzero(&(my_addr.sin_zero),8);

    if(bind(socketfd,(struct sockaddr*) &my_addr,sizeof(struct sockaddr))==-1){
        perror("Unable to bind");exit(1);
    }

    if (listen(socketfd, BACKLOG) == -1) {
        perror("Unable to listen");
        exit(1);
    }
    // sa.sa_handler = sigchld_handler; 
    // sigemptyset(&sa.sa_mask);
    // sa.sa_flags = SA_RESTART;
    // if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    //     perror("sigaction");
    //     exit(1);
    // }
    char hostname[64];size_t size = 64;
    if(gethostname(hostname,size)==-1){
        perror("cant find out server name");
        exit(1);
    }
    system("hostname -I");
    std::cout<<"Server name is:"<<hostname<<std::endl;
    string links[ctr];
    std::vector<vector<pair<int,int>>> adj;
    parse_matrix(argv[1], adj);

    for(int i=0;i<ctr;i++) {       
        sin_size=sizeof(struct sockaddr);
        other_fd[i]=accept(socketfd,(struct sockaddr*)&their_addr,&sin_size);
        if(other_fd[i]==-1){
            perror("Unable to accept");
            exit(1);
        }

        printf("Server got connection from %s\n" , inet_ntoa(their_addr.sin_addr));
            if(!fork()){
                close(socketfd);
                int numbytes=recv(other_fd[i], buffer, MAXDATASIZE-1, 0);
                if ( numbytes== -1) {
                    perror("recv, blah blah");
                    exit(1);
                }
                buffer[numbytes] = '\0';
                links[i]=buffer;
                printf("Text: %s\n",buffer);
                memset(buffer,0,sizeof(buffer));
                // if(send(other_fd,msg.data(),sizeof(msg),0)==-1){ perror("Error while sending msg from server");}
                close(other_fd[i]);exit(0);
            }
        }
        
    for(int i=0;i<ctr;i++){
        string msg="";
        for(int j=0; j < adj[i].size(); j++){
            msg += conv_to_bin((char)('A'+adj[i][j].first) +",") + links[adj[i][j].first] + conv_to_bin(","+to_string(adj[i][j].second)+";");
        }
        msg+=conv_to_bin((char)('A'+i)+",")+ links[i] + conv_to_bin(",0;");
        if(send(other_fd[i],msg.data(),sizeof(msg),0)==-1){ perror("Error while sending msg from server");}
        close(other_fd[i]);
    }



    return 0;
}
#include <stdio.h>
#include <iostream>
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
#include <ctime>

// in seconds
#define AGE_LIMIT 60*25
#define MAXDATASIZE 1000
#define PORT 5000
using namespace std;

// VN to ON connect msg
struct __attribute__((packed)) ConnectMSG{
    uint32_t ip_addr; //4-byte
    uint16_t port; //2-byte
};

// ON to VN Link State Tuple
struct __attribute__((packed)) LSTuple{
    char node;
    uint32_t ip;
    uint16_t port;
    int cost;
};

// VN to VN LSP
struct __attribute__((packed)) Neighcost{
    char node;
    int cost;
};

struct __attribute__((packed)) LSP{
    char origin_id;
    uint32_t seq_no;
    uint8_t num_neigh;
    Neighcost neighbours[26];
};


char node_name;
uint32_t curr_seq_no=0;

time_t edge_time[26][26];
int graph[26][26];

uint32_t latest_seq_no[26]={0};

struct Neigh{
    uint32_t ip;
    uint16_t port;
    int cost;
};
Neigh curr_neighbours[26];

void init_graph(){
    for(int i=0;i<26;i++){
        for(int j=0;j< 26; j++){
            graph[i][j]=-1;
        }
        curr_neighbours[i].cost=-1;
    }
}

void print_graph(){
   int max_node = -1;
    for(int i = 0; i < 26; i++){
        for(int j = 0; j < 26; j++){
            if(graph[i][j] != -1){
                if(i > max_node) max_node = i;
                if(j > max_node) max_node = j;
            }
        }
    }
    for(int i=0;i<max_node;i++){
        for(int j=0;j<i+1;j++){cout<<" _ ";}
        for(int j=i+1;j<=max_node;j++){
            if(time(0) - edge_time[i][j] < AGE_LIMIT)
               if(graph[i][j] >= 0)
                 cout << " " << graph[i][j] << " ";
                else 
                 cout << -1 << " ";

            else
                cout << -1 << " ";
        }

    }
    cout<<"\n\n";
}
void send_my_lsp(int udp_fd){
    if(node_name==0) return;
    LSP lsp;
    lsp.origin_id = node_name;

    curr_seq_no++;
    lsp.seq_no=curr_seq_no;
    lsp.num_neigh=0;

    for(int i=0;i<26;i++){
        if(curr_neighbours[i].cost>0){
            lsp.neighbours[lsp.num_neigh].node= 'A' + i;
            lsp.neighbours[lsp.num_neigh].cost=curr_neighbours[i].cost;
            lsp.num_neigh++;
        }
    }

    for( int i=0;i < 26;i++){
        if(curr_neighbours[i].cost > 0){
            struct sockaddr_in dest_addr;
            dest_addr.sin_family= AF_INET;
            dest_addr.sin_port = htons(curr_neighbours[i].port);
            dest_addr.sin_addr.s_addr=curr_neighbours[i].ip;

            int numbytes=sendto(udp_fd,&lsp, sizeof(lsp),0,(struct sockaddr*) &dest_addr,sizeof(dest_addr));
            if(numbytes==-1){
                perror("sendto LSP failed");
            }
        }
    }
}

int main(int argc , char* argv[]){

    assert(argc == 4);

    init_graph();
    int sockfd, numbytes;
    char buffer[MAXDATASIZE];
    struct hostent *he;
    struct sockaddr_in their_addr;

    // TCP ON to VN connection
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    // char hostname[100];
    // gethostname(hostname,(size_t)(100));
    // printf("text: %s \n",hostname);
    their_addr.sin_family = AF_INET; 
    their_addr.sin_port = htons(PORT); 
    // their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    inet_pton(AF_INET,argv[1] , &their_addr.sin_addr);
    memset(&(their_addr.sin_zero), 0, 8);

    socklen_t sock_len=sizeof(struct sockaddr);
    if (connect(sockfd, (struct sockaddr *)&their_addr,sock_len) == -1) {
        perror("connect");
        exit(1);
    }
    
    // UDP Socket
    int udp_fd=socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in my_udp_addr;

    memset(&my_udp_addr, 0, sizeof(my_udp_addr));
    my_udp_addr.sin_family=AF_INET;
    my_udp_addr.sin_port=htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[3], &my_udp_addr.sin_addr);
    bind(udp_fd,(struct sockaddr*)& my_udp_addr , sizeof(my_udp_addr));

    //Sending msg
    ConnectMSG connect_msg;
    
    inet_pton(AF_INET, argv[3],&connect_msg.ip_addr);
    connect_msg.port=htons(atoi(argv[2]));
    send(sockfd, &connect_msg, sizeof(connect_msg),0);

    cout<<"Sent Connect msg to ON"<<endl;

    fd_set read_fds;
    int max_fd=max(sockfd,udp_fd)+1;
    while(true){
        FD_ZERO(&read_fds);
        FD_SET(sockfd,&read_fds);
        FD_SET(udp_fd,&read_fds);

        struct timeval tv;
        tv.tv_sec=3; // using 10 sec period
        tv.tv_usec=0;

        if(select(max_fd,&read_fds,NULL,NULL,&tv)==0){
            print_graph();

            // SEnd LSP
            send_my_lsp(udp_fd);
            continue;
        }

        if(FD_ISSET(sockfd,&read_fds)){
            uint32_t num_tuples_net;
            int bytes_read=recv(sockfd,&num_tuples_net,sizeof(num_tuples_net),0);

            if(bytes_read <=0){
                perror("No bytes read");
                exit(1);
            }
            int num_tuples=ntohl(num_tuples_net);
            LSTuple tuples[26];
            for(int i=0;i<num_tuples;i++){
                recv(sockfd,&tuples[i],sizeof(LSTuple),0);
                if(ntohl(tuples[i].cost)==0){
                    node_name=tuples[i].node;
                }
            }
            string s=string(1,node_name)+"=0";

            for(int i=0;i<num_tuples;i++){
                LSTuple t = tuples[i];
                int cost=ntohl(t.cost);
                if(cost>0){
                    int idx=t.node-'A';
                    curr_neighbours[idx].ip=t.ip;
                    curr_neighbours[idx].port=ntohs(t.port);
                    curr_neighbours[idx].cost=cost;

                    graph[node_name - 'A'][idx] = cost;
                    graph[idx][node_name - 'A']=cost;
                    s+=","+string(1,t.node)+ "=" + to_string(cost);
                }
            }
            cout<<"Received Link State: "<<s<<endl;
            send_my_lsp(udp_fd);
        }

        if(FD_ISSET(udp_fd,&read_fds)){
            LSP lsp_msg;
            struct sockaddr_in sender_addr;
            socklen_t socklen=sizeof(sender_addr);

            int recv_len=recvfrom(udp_fd,&lsp_msg,sizeof(lsp_msg),0,(struct sockaddr*) &sender_addr,&socklen);
            if(recv_len > 0){
                int origin_id= lsp_msg.origin_id - 'A';
                if(lsp_msg.seq_no > latest_seq_no[origin_id]){
                    latest_seq_no[origin_id]=lsp_msg.seq_no;

                    for(int i=0;i< lsp_msg.num_neigh;i++){
                        int neigh_id=lsp_msg.neighbours[i].node-'A';
                        int cost=lsp_msg.neighbours[i].cost;
                        graph[origin_id][neigh_id]=cost;
                        graph[neigh_id][origin_id]=cost;

                        edge_time[origin_id][neigh_id]=time(0);
                        edge_time[neigh_id][origin_id]=time(0);
                    }
                    for(int i=0;i<26;i++){
                        if(curr_neighbours[i].cost > 0){
                            if(curr_neighbours[i].ip == sender_addr.sin_addr.s_addr && curr_neighbours[i].port==ntohs(sender_addr.sin_port)){
                                continue;
                            }
                            struct sockaddr_in dest_addr;
                            dest_addr.sin_family=AF_INET;
                            dest_addr.sin_port=htons(curr_neighbours[i].port);
                            dest_addr.sin_addr.s_addr=curr_neighbours[i].ip;

                            int numbytes=sendto(udp_fd,&lsp_msg,recv_len,0,(struct sockaddr*)&dest_addr,sizeof(dest_addr));
                            if(numbytes==-1){
                                perror("sendto forward failed");
                            }
                        }
                    }
                }
            }
        }
    }
}

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>
#define SIZE 10000
int main()
{
    int tcp_client = 0;
    struct sockaddr_in client;
    tcp_client = socket(AF_INET, SOCK_STREAM , 0);
    client.sin_family = AF_INET;
    inet_pton(AF_INET , "127.0.0.1", &client.sin_addr);
    client.sin_port = htons(1025);
    connect(tcp_client , (struct sockaddr *)&client ,sizeof(client));
    int sendconf = send(tcp_client, "fup this is the message for ever and ever", 1024, 0);
    printf("%d", sendconf);
}
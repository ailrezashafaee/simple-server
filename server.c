#include <stdio.h>
#include <sys/socket.h>
#include "ini.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sqlite3.h>    
#include <semaphore.h>
#include<pthread.h> 
#define ISVALIDSOCKET(s) ((s) >= 0)
#define CLOSESOCKET(s) close(s)
#define SOCKETERR() (errno)
#define REQUESTMAX 1024
#define THREADS 10
#define INTBUFFER 10
int clients[THREADS];
sem_t mutex;
typedef union message{
    int num_message;
    char *char_message;
}Message;
typedef struct
{
    int port;
} Address;
typedef struct
{
    const char *p1;
    const char *p2;
} Prefix;
typedef struct
{
    Address address;
    Prefix prefix;
} Configuration;
typedef struct args_thread
{
    int socket;
    Prefix prefix;
    sqlite3 *db;
    int id;
} ARGS;

static int load_callback(int argc, char **argv, char **azColName)
{
    int number;
    time_t t;
    for (int i = 0; i < argc; i++)
    {
        //casting operation  :
        if(i == 1)
        {
            number = argv[i]?atoi(argv[i]) :0;
            printf("%s = %d\n", azColName[i], number);
        }
        if(i == 2)
        {
            t = argv[i] ? (time_t)atoi(argv[i]) : 0;
            printf("%s = %s\n", azColName[i],ctime(&t));
        }
    }
    printf("\n");
    return 0;
}
static int handler(void *config, const char *section, const char *name,
                   const char *value)
{
    Configuration *pconfig = (Configuration *)config;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

    if (MATCH("address", "port"))
    {
        pconfig->address.port = atoi(value);
    }
    else if (MATCH("prefix", "p1"))
    {
        pconfig->prefix.p1 = strdup(value);
    }
    else if (MATCH("prefix", "p2"))
    {
        pconfig->prefix.p2 = strdup(value);
    }
    else
    {
        return 0;
    }
    return 1;
}
void *handle_load(void *args_thread)
{  
    sqlite3 *db =(sqlite3*)args_thread;
    char *errMsg = 0;
    int rc;
    char *sql = "SELECT * FROM NUMBERS";
    while(1)
    {
        sleep(10);
        rc = sqlite3_exec(db, sql, load_callback, NULL, &errMsg);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", errMsg);
            sqlite3_free(errMsg);
            pthread_exit(0);
            return 0;
        }
        else
        {
            fprintf(stdout, "Operation done successfully\n");
        }
    }
    pthread_exit(0);
    return 0;
}
int subset_search(char message[],const char *sub_message)
{
    int flg =0; 
    int j =0;
    size_t size_m = strlen(message);
    size_t size_s = strlen(sub_message);
    for(int i =0 ; i<size_m; i++)
    {
        if(j == size_s)
        {
            return i;
        }
        if(message[i] == sub_message[j])
        {
            j++;
            flg = 1;
        }else if(flg || message[i] !=' ')
        {
            return -1;
        }  
    }
    return -1;
}
int get_tid()
{
    for (int i = 0; i < THREADS; i++)
    {
        if (clients[i] == 0)
        {
            return i;
        }
    }
    return -1;
}
int creat_tcp_socket(const char *address, const char *port, const char af)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    // configuring the address family
    if (af == '4')
        hints.ai_family = AF_INET;
    else if (af == '6')
    {
        hints.ai_family = AF_INET6;
    }
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *bind_addr;
    getaddrinfo(address, port, &hints, &bind_addr);
    int tcp_socket;
    tcp_socket = socket(bind_addr->ai_family, bind_addr->ai_socktype, bind_addr->ai_protocol);
    // check if socket has made
    if (!ISVALIDSOCKET(tcp_socket))
    {
        fprintf(stderr, "socket creation failed , %d\n", SOCKETERR());
        exit(1);
    }
    // binding socket to address
    if (bind(tcp_socket, bind_addr->ai_addr, bind_addr->ai_addrlen))
    {
        fprintf(stderr, "binding failed , %d", SOCKETERR());
        exit(1);
    }
    freeaddrinfo(bind_addr);
    // listening
    if (listen(tcp_socket, 10) < 0)
    {
        fprintf(stderr, "Listen fail , %d", SOCKETERR());
        exit(1);
    }
    return tcp_socket;
}
char* insert_query(char *message,char*table,time_t t)
{
    char sql_time_buff[20];
    sprintf(sql_time_buff, "%ld", t);
    char sql_query[REQUESTMAX + 50] = "INSERT INTO  ";
    strcat(sql_query , table);
    char *sql_query_rest;
    sql_query_rest = " (message, date) VALUES ('";
    strcat(sql_query, sql_query_rest);
    strcat(sql_query, message);
    sql_query_rest = "',";
    strcat(sql_query, sql_query_rest);
    strcat(sql_query, sql_time_buff);
    sql_query_rest = ");";
    strcat(sql_query, sql_query_rest);
    char *return_query = (char *)malloc(REQUESTMAX + 50);
    strcpy(return_query , sql_query);
    return return_query;
}
void *handel_client(void *parlSocket)
{
    ARGS *args = parlSocket;
    int socket = args->socket;
    sqlite3 *db = (sqlite3 *)(args->db);
    const char *p1 = (const char *)args->prefix.p1;
    const char *p2 = (const char*)args->prefix.p2;
    int id = (int)args->id;
    free(parlSocket);
    //printf("thread %d is started\n",id);
    //sleep(2);
    // reading message
    char *buffer = (char *)malloc(REQUESTMAX);
    int reccng = recv(socket ,buffer,REQUESTMAX, 0);
    if (reccng < 0)
    {
        perror("recv failed in handler");
        free(buffer);
        clients[id] = 0;
        pthread_exit(0);
        return NULL;
    }
    // parsing
    Message client_message;
    int i;
    char temp[REQUESTMAX];
    time_t t;
    time(&t);
    char *sql_query;
    int query_db;
    char *errorMsg;
    //constructing query
    if ((i = subset_search(buffer,p1)) != -1)
    {
        strncpy(temp, buffer + i, REQUESTMAX - i);
        if(strlen(temp)> 10)
        {
            //perror("Integer too long!, saving it as an string\n");
            free(buffer);
            pthread_exit(0);
            close(socket);
            clients[id] = 0;
            return 0;
        }else{
            client_message.num_message = atoi(temp);
            sql_query = insert_query(temp , "NUMBERS",t);
        }
    }
    else if ((i = subset_search(buffer,p2)) != -1)
    {
        strncpy(temp, buffer +i, REQUESTMAX - i);
        client_message.char_message = temp;
        sql_query = insert_query(temp, "MESSAGE", t);
        
    }else{
        perror("Message recieved has invalid type!\n");
        free(buffer);
        close(socket);
        clients[id] = 0;
        pthread_exit(0);
        return 0;
    }
    free(buffer);
    //excuting query in sqlite database
    query_db = sqlite3_exec(db, sql_query, NULL, 0, &errorMsg);
    free(sql_query);
    if (query_db != SQLITE_OK)
    {
        fprintf(stderr, "SQL exec error :%s\n", errorMsg);
        close(0);
        clients[id] = 0;
        pthread_exit(0);
        return 0;
    }
    //printf("Thread %d finished\n",id);
    clients[id] = 0;
    close(socket);
    pthread_exit(0);
    return 0;
}

int main()
{
    //initializing
    Configuration config;
    struct sockaddr_storage server_storage;
    socklen_t addr_size;
    pthread_t threads[THREADS];
    pthread_t loader;
    sem_init(&mutex ,0 , 1);
    sqlite3 *db;
    //opening our database file
    int rc = sqlite3_open("messages.db", &db);
    if(rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    sqlite3 *load_arg = db;
    if (pthread_create(&loader, NULL, handle_load, (void *)load_arg) < 0)
    {
        perror("Thread creation failed in server program");
        free(load_arg);
        return -1;
    }
    // reading fconfig.ini
    if (ini_parse("fconfig.ini", handler, &config) < 0)
    {
        printf("Cannot load the config file!");
        return 1;
    }
    //creating socket using parsed config
    char port[20];
    sprintf(port,"%d",config.address.port);
    int tcp_socket = creat_tcp_socket("127.0.0.1", port, '4');
    int tr_i; 
    ARGS *args;
    while(1)
    {
        tr_i = -1;
        addr_size = sizeof(server_storage);
        args = (ARGS *)malloc(sizeof *args);
        // accepting connection from the waiting queue
        int new_socket = accept(tcp_socket, (struct sockaddr *)&server_storage, &addr_size);
        if (new_socket == -1)
        {
            perror("error on accept");
            //free(args);
            continue;
        }
        //storing args to pass in thread handler
        args->socket = new_socket;
        args->db = db;
        args->prefix.p1 = config.prefix.p1;
        args->prefix.p2 = config.prefix.p2;
        //searching for available stop among threads

        if(get_tid()==-1)//this is where all the spots availabe are occupied, so we must wait until have some free spots
        {
            for(int i =0 ; i < 5 ;i++)
            {
                if(pthread_join(threads[i],NULL)==-1)
                {
                    perror("Pthread join error");
                    continue;
                }
            }
        }
        tr_i = get_tid();
        clients[tr_i] = 1;
        args->id = tr_i;
        //creating a new thread for the new connection
        
        if (pthread_create(&threads[tr_i], NULL, handel_client, (void *)args) < 0)
        {
            perror("Thread creation failed in server program");
            free(args);
            continue;
        }
    }
    return 0;
}

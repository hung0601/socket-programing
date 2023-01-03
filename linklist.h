#include <stdio.h> /* These are the usual header files */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#define MAX_LEN 200
#define BUFF_SIZE 1024

// user account

typedef enum
{
    ZERO,
    FIND,
    RES_FIND,
    UPDATE,
    RES_UPDATE,
    SELECT,
    RES_SELECT,
    CONNECT,
    END_CONNECT,
    SEND_FILE
} msg_type;

typedef enum
{
    STRING,
    STRING_LIST,
    ADDRESS,
    ADDRESS_LIST,
    NONE
} data_type;

typedef union Data
{
    char buff[BUFF_SIZE];
    char filelst[MAX_LEN][BUFF_SIZE];
    struct sockaddr_in addr;
    struct sockaddr_in addrLst[MAX_LEN];

} Data;

typedef struct message
{
    msg_type ms_type;
    data_type dt_type;
    Data value;
    int size;
} Message;

typedef enum
{
    ONLINE,
    OFFLINE,
    BUSY
} Status;

// account linked list node
typedef struct AccountLst
{

    struct sockaddr_in client;
    pthread_t thread_id;
    int conn_sock;
    Status st;
    char fileLst[MAX_LEN][BUFF_SIZE];
    int size;
    struct AccountLst *next;
} AccountLst;

AccountLst *searchByClient(AccountLst *accLst, struct sockaddr_in client);
void addLogLst(AccountLst **accLst, struct sockaddr_in client, int conn_sock);
void deleteLstByClient(AccountLst **accLst, struct sockaddr_in client);
void freeLst(AccountLst **acc); // free memory
Message findClientHaveFile(AccountLst *acc, char *fileName);
#include <stdio.h> /* These are the usual header files */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#define MAX_LEN 20
#define BUFF_SIZE 512
#define FILE_PK_SIZE 4096 * 10

// user account
typedef struct DisplayQueue
{
    char message[MAX_LEN][BUFF_SIZE];
    int size;
} DisplayQueue;

typedef enum
{
    ZERO,
    INIT,
    FIND,
    RES_FIND,
    UPDATE,
    RES_UPDATE,
    SELECT,
    RES_SELECT,
    CONNECT,
    SEND_FILE_NAME,
    SELECT_FILE,
    SEND_FILE,
    END_FILE,
    END_CONNECT,
    END
} msg_type;

typedef enum
{
    NONE,
    STRING,
    STRING_LIST,
    ADDRESS,
    ADDRESS_LIST
} data_type;

typedef struct Data
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
typedef struct FileTranferMsg
{
    msg_type ms_type;
    char buff[FILE_PK_SIZE];
    int size;
} FileTranferMsg;

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
typedef struct ClientLst
{
    struct sockaddr_in client;
    struct ClientLst *next;
} ClientLst;

AccountLst *searchByClient(AccountLst *accLst, struct sockaddr_in client);
void addLogLst(AccountLst **accLst, struct sockaddr_in client, int conn_sock);
void deleteLstByClient(AccountLst **accLst, struct sockaddr_in client);
void freeLst(AccountLst **acc); // free memory
Message findClientHaveFile(AccountLst *acc, char *fileName, int conn);
void switchCaseSelect(AccountLst *clientLst, AccountLst *acc, Message ms);
void switchCaseResponseSelect(AccountLst *clientLst, AccountLst *acc, Message ms);
void addClient(ClientLst **clientLst, struct sockaddr_in client);
void deleteClient(ClientLst **clientLst, struct sockaddr_in client);
struct sockaddr_in findClientById(ClientLst *clientLst, int id);
DisplayQueue *createQueue();
void addToQueue(DisplayQueue *queue, char *buff);
void clearQueue(DisplayQueue *queue);
int isClientHaveFile(char *name);
#include <stdio.h> /* These are the usual header files */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "linklist.h"

#define PORT 5550 /* Port that will be opened */
#define BACKLOG 2 /* Number of allowed connections */

AccountLst *clientLst = NULL;
AccountLst *clientNode = NULL;

int listen_sock, conn_sock; /* file descriptors */
char recv_data[BUFF_SIZE];
int bytes_sent, bytes_received;
struct sockaddr_in server; /* server's address information */
struct sockaddr_in client; /* client's address information */
int sin_size;
sem_t x;

void *sendUpdateReq(void *vargp)
{
    Message ms;
    AccountLst *ptr;
    while (1)
    {
        sleep(10);
        ms.ms_type = UPDATE;
        ms.dt_type = NONE;
        if (clientLst != NULL)
        {
            ptr = clientLst;
            while (ptr != NULL)
            {
                bytes_sent = send(ptr->conn_sock, &ms, sizeof(ms), 0);
                if (bytes_sent <= 0)
                {
                    printf("\nConnection closed!\n");
                    return NULL;
                }

                ptr = ptr->next;
            }
        }
    }
    return NULL;
}

void *listenMessage(void *varqp)
{
    AccountLst *acc;
    Message ms;
    Message send_msg;
    int bytes_received;
    int bytes_sent;
    while (1)
    {
        memset( &ms, 0, sizeof(Message));


        acc = (AccountLst *)varqp;
        bytes_received = recv(acc->conn_sock, &ms, sizeof(ms), 0); // blocking
        if (bytes_received <= 0)
        {
            continue;
        }
        else
        {
            // printf("%d\n", ms.ms_type);
            switch (ms.ms_type)
            {
            case RES_UPDATE:
                if (ms.dt_type == STRING_LIST)
                {
                    // printf("Recv file lst from %s:%d\n", inet_ntoa(acc->client.sin_addr), ntohs(acc->client.sin_port));

                    // printf("%d\n", ms.size);;

                    for (int i = 0; i < ms.size; i++)
                    {
                        if (ms.value.filelst[i] != NULL && strcmp(ms.value.filelst[i], ""))
                        {
                            // printf("%s\n", ms.value.filelst[i]);
                            strcpy(acc->fileLst[i], ms.value.filelst[i]);
                        }
                    }
                    acc->size = ms.size;
                }
                break;
            case FIND:
                send_msg = findClientHaveFile(clientLst, ms.value.buff);
                // printf("thread %ld connsock %d. to %s:%d\n", acc->thread_id, acc->conn_sock, inet_ntoa(acc->client.sin_addr), ntohs(acc->client.sin_port));
                // for (int i = 0; i < send_msg.size; i++)
                // {
                //     printf("    %s:%d\n", inet_ntoa(send_msg.value.addrLst[i].sin_addr), ntohs(send_msg.value.addrLst[i].sin_port));
                // }
                bytes_sent = send(acc->conn_sock, &send_msg, sizeof(send_msg), 0);
                if (bytes_sent <= 0)
                {
                    printf("\nConnection closed!\n");
                    return NULL;
                }
                break;

            case SELECT:
            {
                if(ms.dt_type == ADDRESS) {
                    printf("Client %s:%d request connection to client %s:%d\n", 
                    inet_ntoa(acc->client.sin_addr), ntohs(acc->client.sin_port),
                    inet_ntoa(ms.value.addr.sin_addr), ntohs(ms.value.addr.sin_port));
                
                    clientNode = searchByClient(clientLst, ms.value.addr);

                    // tìm kiếm client B, nếu thấy mà đang ONLINE thì gửi yêu cầu connect từ client A,
                    // nếu trạng thái là BUSY, thì gửi response về client A là client B đang bận 
                    // Nếu không tìm thấy client B, thì gửi response về client A 
                    if (clientNode != NULL) {
                        if(clientNode->st == ONLINE) {
                            send_msg.ms_type = SELECT;
                            send_msg.dt_type = ADDRESS;
                            send_msg.value.addr.sin_family = AF_INET;
                            send_msg.value.addr.sin_port = acc->client.sin_port;
                            send_msg.value.addr.sin_addr.s_addr = acc->client.sin_addr.s_addr;

                            bytes_sent = send(clientNode->conn_sock, &send_msg, sizeof(send_msg), 0);
                            if (bytes_sent <= 0)
                            {
                                printf("\nConnection closed!\n");
                                return NULL;
                            }
                            break;

                        }
                        else {
                            send_msg.ms_type = RES_SELECT;
                            send_msg.dt_type = STRING;
                            strcpy(send_msg.value.buff, "busy");

                            bytes_sent = send(acc->conn_sock, &send_msg, sizeof(send_msg), 0);
                        }
                } 
                else {
                    memset( &send_msg, 0, sizeof(Message));
                    send_msg.ms_type = RES_SELECT;
                    send_msg.dt_type = STRING;
                    strcpy(send_msg.value.buff, "null");
                    bytes_sent = send(acc->conn_sock, &send_msg, sizeof(send_msg), 0);

                }
                }
            }
            break;


            // response từ client B 
            case RES_SELECT:
            {
                memset( &send_msg, 0, sizeof(Message));
                send_msg.ms_type = RES_SELECT;
                send_msg.dt_type = STRING;

                if (bytes_received <= 0)
                {
                    continue;
                }

                if (ms.dt_type == ADDRESS) {

                    strcpy(send_msg.value.buff, "yes");

                    acc->st= BUSY;

                    clientNode = searchByClient(clientLst, ms.value.addr);
                    if(clientNode == NULL) 
                        printf("Da xay ra loi\n");

                    clientNode->st= BUSY;

                }else
                {
                    strcpy(send_msg.value.buff, "no");
                } 

                printf("Value %s:%d \n", 
                    inet_ntoa(ms.value.addr.sin_addr), ntohs(ms.value.addr.sin_port));
                printf("%s\n", send_msg.value.buff);

                
                // memset( &ms, 0, sizeof(Message));
                
                clientNode = searchByClient(clientLst, ms.value.addr);
                if(clientNode == NULL) 
                    printf("Da xay ra loi\n");

                bytes_sent = send(clientNode->conn_sock, &send_msg, sizeof(send_msg), 0);
                if (bytes_sent <= 0)
                {
                    printf("\nConnection closed!\n");
                    return NULL;
                }
            }

            break;

            case END_CONNECT:
                acc->st = ONLINE;
            break;

            // cần tạo ra 1 case mới để nhận res_select từ client , sau khi nhận xong thì sẽ gửi đến client yêu cầu , còn client nào mà không được đồng ý, 
            // thì tiếp tục đợi, nếu mãi mà không  thấy đồng ý thì nhập lại yêu cầu cho phía server 

            default:
                break;
            }
        }
    }
    
    return NULL;
}





int main()
{
    Message ms;
    pthread_t thread_id;
    // Step 1: Construct a TCP socket to listen connection request
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    { /* calls socket() */
        perror("\nError: ");
        return 0;
    }

    // Step 2: Bind address to socket`
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);              /* Remember htons() from "Conversions" section? =) */
    server.sin_addr.s_addr = htonl(INADDR_ANY); /* INADDR_ANY puts your IP address automatically */
    if (bind(listen_sock, (struct sockaddr *)&server, sizeof(server)) == -1)
    { /* calls bind() */
        perror("\nError: ");
        return 0;
    }

    // Step 3: Listen request from client
    if (listen(listen_sock, BACKLOG) == -1)
    { /* calls listen() */
        perror("\nError: ");
        return 0;
    }
    pthread_create(&thread_id, NULL, sendUpdateReq, NULL);
    // Step 4: Communicate with client
    while (1)
    {
        // accept request
        sin_size = sizeof(struct sockaddr_in);
        if ((conn_sock = accept(listen_sock, (struct sockaddr *)&client, &sin_size)) == -1)
            perror("\nError: ");

        if (searchByClient(clientLst, client) == NULL)
            addLogLst(&clientLst, client, conn_sock);
        printf("You got a connection from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port)); /* prints client's IP */
        clientNode = searchByClient(clientLst, client);
        pthread_create(&clientNode->thread_id, NULL, listenMessage, (void *)clientNode);
        // start conversation
    }
    pthread_join(thread_id, NULL);
    close(listen_sock);
    return 0;
}

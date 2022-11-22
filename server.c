#include <stdio.h> /* These are the usual header files */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
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

void *sendUpdateReq(void *vargp)
{
    Message ms;
    AccountLst *ptr;
    while (1)
    {
        sleep(5);
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
    AccountLst *acc = (AccountLst *)varqp;
    Message ms;
    int bytes_received;
    while (1)
    {
        // printf("con: %d\n", acc->conn_sock);
        bytes_received = recv(acc->conn_sock, &ms, sizeof(ms), 0); // blocking
        if (bytes_received <= 0)
        {
            // printf("Connection closed1\n");
            continue;
        }
        else
        {
            switch (ms.ms_type)
            {
            case RES_UPDATE:
                if (ms.dt_type == STRING_LIST)
                {
                    printf("Recv file lst from %s:%d\n", inet_ntoa(acc->client.sin_addr), ntohs(acc->client.sin_port));
                    for (int i = 0; i < ms.size; i++)
                    {

                        if (ms.value.filelst[i] != NULL && strcmp(ms.value.filelst[i], ""))
                        {
                            printf("%s\n", ms.value.filelst[i]);
                            strcpy(acc->fileLst[i], ms.value.filelst[i]);
                        }
                    }
                }
                break;

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

    // Step 2: Bind address to socket
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

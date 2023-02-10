#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "linklist.h"

AccountLst *searchByClient(AccountLst *accLst, struct sockaddr_in client)
{
    AccountLst *ptr = accLst;
    while (ptr != NULL)
    {
        if (!strcmp(inet_ntoa(client.sin_addr), inet_ntoa(ptr->client.sin_addr)) &&
            ntohs(client.sin_port) == ntohs(ptr->client.sin_port))
            return ptr;
        ptr = ptr->next;
    }
    return NULL;
}

void addLogLst(AccountLst **accLst, struct sockaddr_in client, int conn_sock)
{
    AccountLst *new = (AccountLst *)malloc(sizeof(AccountLst));
    new->next = NULL;
    new->client = client;
    new->conn_sock = conn_sock;
    new->st = ONLINE;

    if (*accLst == NULL)
        *accLst = new;
    else
    {
        AccountLst *lastNode = *accLst;

        while (lastNode->next != NULL)
        {
            lastNode = lastNode->next;
        }

        lastNode->next = new;
    }
}

void deleteLstByClient(AccountLst **accLst, struct sockaddr_in client)
{
    AccountLst *ptr = *accLst;
    AccountLst *dele;
    if (ptr != NULL && !strcmp(inet_ntoa(client.sin_addr), inet_ntoa(ptr->client.sin_addr)) &&
        ntohs(client.sin_port) == ntohs(ptr->client.sin_port))
    {
        *accLst = ptr->next;
        free(ptr);
        return;
    }
    else
    {

        while (ptr->next != NULL)
        {
            if (!strcmp(inet_ntoa(client.sin_addr), inet_ntoa(ptr->client.sin_addr)) &&
                ntohs(client.sin_port) == ntohs(ptr->client.sin_port))
            {
                dele = ptr->next;
                ptr->next = ptr->next->next;
                free(dele);
                return;
            }
            ptr = ptr->next;
        }
    }
}

void freeLst(AccountLst **acc)
{
    AccountLst *ptr = *acc;
    AccountLst *temp = NULL;
    while (ptr != NULL)
    {
        temp = ptr->next;
        free(ptr);
        ptr = temp;
    }
    *acc = NULL;
}

Message findClientHaveFile(AccountLst *acc, char *fileName, int conn)
{
    Message msg;
    AccountLst *ptr;
    ptr = acc;
    msg.ms_type = RES_FIND;
    msg.dt_type = ADDRESS_LIST;
    msg.size = 0;
    while (ptr != NULL)
    {
        for (int i = 0; i < ptr->size; i++)
        {

            if (!strcmp(ptr->fileLst[i], fileName) && conn != ptr->conn_sock && ptr->st == ONLINE)
            {
                msg.value.addrLst[msg.size++] = ptr->client;
                // printf("    %s:%d\n", inet_ntoa(ptr->client.sin_addr), ntohs(ptr->client.sin_port));
                break;
            }
        }
        ptr = ptr->next;
    }
    return msg;
}

void switchCaseSelect(AccountLst *clientLst, AccountLst *acc, Message ms)
{
    AccountLst *clientNode = NULL;
    Message send_msg;
    int bytes_received;
    int bytes_sent;

    if (ms.dt_type == ADDRESS)
    {
        printf("Client %s:%d request connection to client %s:%d\n",
               inet_ntoa(acc->client.sin_addr), ntohs(acc->client.sin_port),
               inet_ntoa(ms.value.addr.sin_addr), ntohs(ms.value.addr.sin_port));

        clientNode = searchByClient(clientLst, ms.value.addr);

        // tìm kiếm client B, nếu thấy mà đang ONLINE thì gửi yêu cầu connect từ client A,
        // nếu trạng thái là BUSY, thì gửi response về client A là client B đang bận
        // Nếu không tìm thấy client B, thì gửi response về client A
        if (clientNode != NULL)
        {
            if (clientNode->st == ONLINE)
            {
                send_msg.ms_type = SELECT;
                send_msg.dt_type = ADDRESS;
                send_msg.value.addr.sin_family = AF_INET;
                send_msg.value.addr.sin_port = acc->client.sin_port;
                send_msg.value.addr.sin_addr.s_addr = acc->client.sin_addr.s_addr;

                bytes_sent = send(clientNode->conn_sock, &send_msg, sizeof(send_msg), 0);
                if (bytes_sent <= 0)
                {
                    printf("\nConnection closed!\n");
                    return;
                }
            }
            else if (clientNode->st == BUSY)
            {
                send_msg.ms_type = RES_SELECT;
                send_msg.dt_type = STRING;
                strcpy(send_msg.value.buff, "busy");

                bytes_sent = send(acc->conn_sock, &send_msg, sizeof(send_msg), 0);
            }
            else
            {
                send_msg.ms_type = RES_SELECT;
                send_msg.dt_type = STRING;
                strcpy(send_msg.value.buff, "offline");

                bytes_sent = send(acc->conn_sock, &send_msg, sizeof(send_msg), 0);
            }
        }
        else
        {
            memset(&send_msg, 0, sizeof(Message));
            send_msg.ms_type = RES_SELECT;
            send_msg.dt_type = STRING;
            strcpy(send_msg.value.buff, "null");
            bytes_sent = send(acc->conn_sock, &send_msg, sizeof(send_msg), 0);
        }
    }
}

void switchCaseResponseSelect(AccountLst *clientLst, AccountLst *acc, Message ms)
{
    AccountLst *clientNode = NULL;
    Message send_msg;
    int bytes_received;
    int bytes_sent;

    memset(&send_msg, 0, sizeof(Message));
    send_msg.ms_type = RES_SELECT;
    send_msg.dt_type = ADDRESS;

    if (ms.dt_type == ADDRESS_LIST)
    {
        send_msg.size = 1;
        send_msg.value.addr = ms.value.addrLst[1];
        strcpy(send_msg.value.buff, "yes");

        acc->st = BUSY;

        clientNode = searchByClient(clientLst, ms.value.addrLst[0]);
        if (clientNode == NULL)
            printf("Da xay ra loi\n");

        clientNode->st = BUSY;
    }
    else
    {
        send_msg.size = 0;
        strcpy(send_msg.value.buff, "no");
    }

    // memset( &ms, 0, sizeof(Message));

    clientNode = searchByClient(clientLst, ms.value.addrLst[0]);
    if (clientNode == NULL)
        printf("Da xay ra loi\n");

    bytes_sent = send(clientNode->conn_sock, &send_msg, sizeof(send_msg), 0);
    if (bytes_sent <= 0)
    {
        printf("\nConnection closed!\n");
        return;
    }
}

void addClient(ClientLst **clientLst, struct sockaddr_in client)
{
    ClientLst *new = (ClientLst *)malloc(sizeof(ClientLst));
    new->next = NULL;
    new->client = client;

    if (*clientLst == NULL)
        *clientLst = new;
    else
    {
        ClientLst *lastNode = *clientLst;

        while (lastNode->next != NULL)
        {
            lastNode = lastNode->next;
        }

        lastNode->next = new;
    }
}

void deleteClient(ClientLst **clientLst, struct sockaddr_in client)
{
    ClientLst *ptr = *clientLst;
    ClientLst *dele;
    if (ptr != NULL && !strcmp(inet_ntoa(client.sin_addr), inet_ntoa(ptr->client.sin_addr)) &&
        ntohs(client.sin_port) == ntohs(ptr->client.sin_port))
    {
        *clientLst = ptr->next;
        free(ptr);
        return;
    }
    else
    {
        while (ptr->next != NULL)
        {
            if (!strcmp(inet_ntoa(client.sin_addr), inet_ntoa(ptr->client.sin_addr)) &&
                ntohs(client.sin_port) == ntohs(ptr->client.sin_port))
            {
                dele = ptr->next;
                ptr->next = ptr->next->next;
                free(dele);
                return;
            }
            ptr = ptr->next;
        }
    }
}
struct sockaddr_in findClientById(ClientLst *clientLst, int id)
{
    ClientLst *ptr;
    ptr = clientLst;
    int i = 0;
    while (ptr != NULL)
    {
        i++;
        if (id == i)
            return ptr->client;
        ptr = ptr->next;
    }
}
DisplayQueue *createQueue()
{
    DisplayQueue *new = (DisplayQueue *)malloc(sizeof(DisplayQueue));
    new->size = 0;
    return new;
}
void addToQueue(DisplayQueue *queue, char *buff)
{
    strcpy(queue->message[queue->size++], buff);
}
void clearQueue(DisplayQueue *queue)
{
    memset(queue, 0, sizeof(DisplayQueue));
}
int isClientHaveFile(char *name)
{
    DIR *dr;
    struct dirent *de;
    int i;
    dr = opendir("./doc");
    i = 0;
    if (dr == NULL) // opendir returns NULL if couldn't open directory
    {
        printf("Could not open current directory.\n");
        return 0;
    }

    while ((de = readdir(dr)) != NULL)
    {
        if (de->d_type == 8)
        {
            if (!strcmp(name, de->d_name))
            {
                closedir(dr);
                return 1;
            }
        }
    }
    // sprintf(send_msg.value.filelst[0], "%d", countFileLst);
    closedir(dr);
    return 0;
}
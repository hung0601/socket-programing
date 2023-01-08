#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

Message findClientHaveFile(AccountLst *acc, char *fileName)
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

            if (!strcmp(ptr->fileLst[i], fileName))
            {
                msg.value.addrLst[msg.size++] = ptr->client;
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
            else
            {
                send_msg.ms_type = RES_SELECT;
                send_msg.dt_type = STRING;
                strcpy(send_msg.value.buff, "busy");

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
    send_msg.dt_type = STRING;


    if (ms.dt_type == ADDRESS)
    {

        strcpy(send_msg.value.buff, "yes");

        acc->st = BUSY;

        clientNode = searchByClient(clientLst, ms.value.addr);
        if (clientNode == NULL)
            printf("Da xay ra loi\n");

        clientNode->st = BUSY;
    }
    else
    {
        strcpy(send_msg.value.buff, "no");
    }

    printf("Value %s:%d \n",
           inet_ntoa(ms.value.addr.sin_addr), ntohs(ms.value.addr.sin_port));
    printf("%s\n", send_msg.value.buff);

    // memset( &ms, 0, sizeof(Message));

    clientNode = searchByClient(clientLst, ms.value.addr);
    if (clientNode == NULL)
        printf("Da xay ra loi\n");

    bytes_sent = send(clientNode->conn_sock, &send_msg, sizeof(send_msg), 0);
    if (bytes_sent <= 0)
    {
        printf("\nConnection closed!\n");
        return;
    }
}

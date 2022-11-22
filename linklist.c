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
    // create a new node
    AccountLst *new = (AccountLst *)malloc(sizeof(AccountLst));
    new->next = NULL;
    new->client = client;
    new->conn_sock = conn_sock;
    new->st = ONLINE;

    // if head is NULL, it is an empty list
    if (*accLst == NULL)
        *accLst = new;
    // Otherwise, find the last node and add the newNode
    else
    {
        AccountLst *lastNode = *accLst;

        // last node's next address will be NULL.
        while (lastNode->next != NULL)
        {
            lastNode = lastNode->next;
        }

        // add the newNode at the end of the linked list
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

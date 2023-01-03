#include <stdio.h> /* These are the usual header files */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include "linklist.h"

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 5550

int client_sock, countClientReqConn=0;
struct sockaddr_in server_addr;
struct sockaddr_in clientReqConn[MAX_LEN];


void *getSearchFileName(void *vargp)
{
    char fileName[BUFF_SIZE], port[BUFF_SIZE], address[BUFF_SIZE];
    int bytes_sent;
    Message ms;
    int choice, choiceOfConnect, choiceClient=0;
    printf("Welcome to P2P file tranfer application\n");
    while (1)
    {
        printf("------------------------Menu---------------------\n");
        printf("1. Input file name to search\n");
        printf("2. Input IP address and Port of client to request connect\n");
        printf("3. Answer connection request\n");
        printf("4. Exit\n");
        printf("-------------------------------------------------\n\n");

        printf("Please enter your choice: ");
        scanf("%d", &choice);
        while(getchar() != '\n');


        switch (choice)
        {
            case 1:
            {
                printf("Input file name and press ENTER to search:\n");

                memset(fileName, '\0', (strlen(fileName)) + 1);
                fgets(fileName, BUFF_SIZE, stdin);
                fileName[strlen(fileName) - 1] = '\0';
                while(getchar() != '\n');
                    
                ms.ms_type = FIND;
                ms.dt_type = STRING;
                strcpy(ms.value.buff, fileName);
                bytes_sent = send(client_sock, &ms, sizeof(ms), 0);
                if (bytes_sent <= 0)
                {
                    printf("Connection closed!\n");
                    exit(0);
                }
            }
            break;

    
            case 2: 
                printf("Input IP address Of client:\n");

                memset(address, '\0', (strlen(address)) + 1);
                fgets(address, BUFF_SIZE, stdin);
                address[strlen(address) - 1] = '\0';


                printf("Input Port Of client:\n");

                memset(port, '\0', (strlen(port)) + 1);
                fgets(port, BUFF_SIZE, stdin);
                port[strlen(port) - 1] = '\0';

                ms.ms_type = SELECT;
                ms.dt_type = ADDRESS;
                ms.value.addr.sin_family = AF_INET;
                ms.value.addr.sin_port = htons(atoi(port));
                ms.value.addr.sin_addr.s_addr = inet_addr(address);
                bytes_sent = send(client_sock, &ms, sizeof(ms), 0);
                if (bytes_sent <= 0)
                {
                        printf("Connection closed!\n");
                        exit(0);
                }
            
            break;

            // cần phải trả lời đồng ý hay từ chối cho kết nối ? và kết nối với client nào, thì nhập vào từ bàn phím
            // phải trả lời cho từng trường hợp đồng ý hay từ chối 
            
            case 3: 
                // port và Ip address này phải thuộc mảng các client đã gửi yêu cầu kết nối , nếu không thì ,
                memset( &ms, 0, sizeof(Message));
                ms.ms_type = RES_SELECT;


                printf("Clients request connection:\n");
                for(int i= 0; i< countClientReqConn; i++) {
                    // printf(" %d - %d\n", atoi(port), ntohs(clientReqConn[i].sin_port));
                    // printf(" %s - %s\n", inet_ntoa(ms.value.addr.sin_addr), inet_ntoa(clientReqConn[i].sin_addr));
                    printf("    %d. %s:%d\n", i+1,  inet_ntoa(clientReqConn[i].sin_addr), ntohs(clientReqConn[i].sin_port));
                } 

                printf("\nThe client you choose is: ");
                scanf("%d", &choiceClient);

                if (choiceClient > countClientReqConn) 
                {
                    printf("Invalid selection.\n");
                    continue;
                }
                else {
                    ms.value.addr.sin_family = AF_INET;
                    ms.value.addr.sin_port = clientReqConn[choiceClient -1].sin_port;
                    ms.value.addr.sin_addr.s_addr = clientReqConn[choiceClient - 1].sin_addr.s_addr;
                }

                printf("\n1. Accept\n");
                printf("2. Reject\n\n");
                printf("Your choice is : ");
                scanf("%d", &choiceOfConnect);

                if (choiceOfConnect == 1){
                    ms.dt_type = ADDRESS;
                }
                else 
                    ms.dt_type = NONE;
                

                bytes_sent = send(client_sock, &ms, sizeof(ms), 0);

                

            break;

            case 4: 
                // Thoát nhưng chưa xóa dữ liệu bên server 
                printf("Goodbye.\n");
                exit(0);
            break;

            default: 
                printf("Choice does not exist.\n");
        }
    }
    
}

void getFileLst(char **fileLst)
{
    struct dirent *de;
    DIR *dr = opendir(".");
    int i = 0;
    if (dr == NULL) // opendir returns NULL if couldn't open directory
    {
        printf("Could not open current directory");
        return;
    }
    while ((de = readdir(dr)) != NULL)
    {
        if (de->d_type == 8)
            strcpy(fileLst[i++], de->d_name);
        if (i >= MAX_LEN)
        {
            printf("out of array length\n");
            break;
        }
    }
    closedir(dr);
}

int main()
{
    struct dirent *de;
    DIR *dr;
    pthread_t thread_id;
    /* server's address information */
    char buff[BUFF_SIZE];
    int msg_len, bytes_sent, bytes_received;
    int i = 0;

    Message ms, send_msg;
    // Step 1: Construct socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);

    // Step 2: Specify server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    // Step 3: Request to connect server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
    {
        printf("\nError!Can not connect to sever! Client exit imediately! ");
        return 0;
    }

    pthread_create(&thread_id, NULL, getSearchFileName, NULL);

    while (1)
    {
        bytes_received = recv(client_sock, &ms, sizeof(ms), 0); // blocking
        if (bytes_received <= 0)
        {
            printf("\nConnection closed\n");
            break;
        }
        else
        {
            switch (ms.ms_type)
            {
            case UPDATE:
                send_msg.dt_type = STRING_LIST;
                send_msg.ms_type = RES_UPDATE;
                send_msg.size = 0;
                dr = opendir("./doc");
                int i = 0;
                if (dr == NULL) // opendir returns NULL if couldn't open directory
                {
                    printf("Could not open current directory");
                    break;
                }
                while ((de = readdir(dr)) != NULL)
                {
                    if (de->d_type == 8)
                        strcpy(send_msg.value.filelst[i++], de->d_name);
                    send_msg.size = i;
                    if (i >= MAX_LEN)
                    {
                        printf("out of array length\n");
                        break;
                    }
                }

                closedir(dr);
                bytes_sent = send(client_sock, &send_msg, sizeof(send_msg), 0);
                if (bytes_sent <= 0)
                {
                    printf("Connection closed!\n");
                    exit(0);
                }
                break;
            case RES_FIND:
                printf("\nClient list:\n");
                for (int i = 0; i < ms.size; i++)
                {
                    printf("    %s:%d\n", inet_ntoa(ms.value.addrLst[i].sin_addr), ntohs(ms.value.addrLst[i].sin_port));
                }
                break;

            case SELECT:
                printf("\nYou get a connection request to download files from client %s:%d.\n\n",
                inet_ntoa(ms.value.addr.sin_addr), ntohs(ms.value.addr.sin_port));
                clientReqConn[countClientReqConn].sin_family =  AF_INET;
                clientReqConn[countClientReqConn].sin_port =  ms.value.addr.sin_port;
                clientReqConn[countClientReqConn].sin_addr.s_addr =  ms.value.addr.sin_addr.s_addr;

                countClientReqConn++;
            break;

            case RES_SELECT:
                // printf("\nres_select: %s\n",ms.value.buff);
                printf("\nResponse of request connection: ");
                if (!strcmp(ms.value.buff, "yes"))
                    printf("That client has approved to connect to download the file.\n");
                else if  (!strcmp(ms.value.buff, "no"))
                    printf("That client refused to connect to download the file.\n");
                else if (!strcmp(ms.value.buff, "busy"))
                    printf("That client is busy.\n");
                else 
                    printf("No matching client found.\n");

            break;
            default:
                break;
            }
        }
    }
    close(client_sock);
    return 0;
}

// nếu như đang gửi dữ liệu lên server để update các file mà client đang giữ,  mà người dùng gửi yêu cầu tìm file đến thì server có vẻ sẽ bị lỗi
// có thể nó đang gặp lỗi dòng 100 bên client, 138 bên server.
#include <stdio.h> /* These are the usual header files */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <stdbool.h>
#include <openssl/md5.h>
#include "linklist.h"

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 5550
#define P2P_SERVER_PORT 5551

pthread_t thread_id;
int client_sock, countClientReqConn = 0;
struct sockaddr_in server_addr;
struct sockaddr_in clientReqConn[MAX_LEN];
char fileLst[MAX_LEN][BUFF_SIZE];
int countFileLst = 0;
bool waitForReplyFromOtherClient = false;
pthread_t threadOfClientRequestConn, threadOfClientReceiveConn;

void hashFile(char *filename, char *c)
{
    int i;
    FILE *inFile = fopen(filename, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];

    if (inFile == NULL)
    {
        printf("%s can't be opened.\n", filename);
        exit(0);
    }

    MD5_Init(&mdContext);
    while ((bytes = fread(data, 1, 1024, inFile)) != 0)
        MD5_Update(&mdContext, data, bytes);
    MD5_Final(c, &mdContext);
    for (i = 0; i < MD5_DIGEST_LENGTH; i++)
        printf("%c", c[i]);
    printf(" - %ld - %s\n", strlen(c), filename);
    fclose(inFile);
}


// chỉ cho phép 2 client kết nối 2 phút với nhau, 
// và phải end connect trước khi thời điểm trên,
// vì luồng chính của từng client sleep 2 phút
// nếu không end connect thì lúc này 2 client sẽ là trạng thái BUSY 
// sẽ không thể nhận được bất kì thông điệp nào từ server. 

void *clientSendReqDown()
{
    printf("\nYou have 2 minutes to connect, you must disconnect before the above time.\n");

    int choice, choiceInside;
    int client_sock_p2p;
    struct sockaddr_in address;
    int size;
    Message send_msg, recv_msg;
    int bytes_sent, bytes_received;
    char fileListClientB[MAX_LEN][BUFF_SIZE];
    char nameFile[BUFF_SIZE];
    char hashFileReturn[BUFF_SIZE];

    FILE *fp;

    // Creating socket file descriptor
    if ((client_sock_p2p = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = server_addr.sin_addr.s_addr;
    address.sin_port = htons(P2P_SERVER_PORT);

    if (connect(client_sock_p2p, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("\nConnection Failed \n");
    }

    bytes_received = recv(client_sock_p2p, &recv_msg, sizeof(recv_msg), 0); // blocking
    if (bytes_received <= 0)
    {
        perror("recv");
    }

    if (recv_msg.ms_type == CONNECT)
    {

        for (int i = 0; i < recv_msg.size; i++)
        {
            strcpy(fileListClientB[i], recv_msg.value.filelst[i]);
        }
        size = recv_msg.size;
        do
        {
            memset(&send_msg, 0, sizeof(Message));
            printf("-------------Menu Request Other Client-------------\n");
            printf("1. Select file to download.\n");
            printf("0. End and disconnecting.\n");
            printf("--------------------------------------------------\n\n");

            printf("(Send Other Client) Please enter your choice: ");
            scanf("%d", &choice);

            switch (choice)
            {
            case 1:
            {
                printf("\nList file of that client:\n");
                for (int i = 0; i < size; i++)
                {
                    printf("    %d.  %s\n", i + 1, fileListClientB[i]);
                }
                printf("\n(Send Other Client Inside)Please enter your choice: ");
                scanf("%d", &choiceInside);

                memset(&send_msg, 0, sizeof(Message));
                send_msg.ms_type = SELECT_FILE;
                send_msg.dt_type = STRING_LIST;

                strcpy(send_msg.value.buff, fileListClientB[choiceInside - 1]);
                printf("\nThe file you selected is : %s\n", fileListClientB[choiceInside - 1]);

                bytes_sent = send(client_sock_p2p, &send_msg, sizeof(send_msg), 0);
                memset(nameFile, '\0', strlen(nameFile) + 1);
                strcpy(nameFile, "doc/");
                strcat(nameFile, fileListClientB[choiceInside - 1]);
                fp = fopen(nameFile, "a+");

                while (1)
                {
                    memset(&recv_msg, 0, sizeof(recv_msg));

                    bytes_received = recv(client_sock_p2p, &recv_msg, sizeof(recv_msg), 0); // blocking
                    if (bytes_received <= 0)
                    {
                        printf("\nConnection closed\n");
                        break;
                    }
                    else
                    {
                        switch (recv_msg.ms_type)
                        {
                        case SEND_FILE:
                        {
                            fprintf(fp, "%s", recv_msg.value.buff);
                        }
                        break;

                        // nhận được mã hash từ client kia kèm theo tín hiệu end file
                        // lúc này sẽ mã hóa file mới nhận
                        // và so sánh 
                        // nếu giống nhau thì thông báo thành công và ngược lại
                        case END_FILE:
                            fclose(fp);
                            // printf("value: %s - %ld\n", recv_msg.value.buff, strlen(recv_msg.value.buff));
                            hashFile(nameFile, hashFileReturn);
                            if (!strcmp(recv_msg.value.buff, hashFileReturn))
                            {
                                printf("\nSuccessfully sent.\n");
                            }
                            else {
                                printf("\nAn error occurred on the transmission line.\n");
                            }
                            break;
                        default:
                            break;
                        }
                    }

                    if (recv_msg.ms_type == END_FILE)
                        break;
                }
            }

            break;
            case 0:
            {
                printf("\nEND CONNECT.\n");

                send_msg.ms_type = END_CONNECT;
                send_msg.dt_type = NONE;

                bytes_sent = send(client_sock_p2p, &send_msg, sizeof(send_msg), 0);
                bytes_sent = send(client_sock, &send_msg, sizeof(send_msg), 0);
            }
            break;
            default:
                printf("\nWrong choice\n");
            }
        } while (choice);
    }
    else
    {
        printf("\nError when connecting.\n");
    }
    close(client_sock_p2p);
}

void *clientRecvReqDown()
{
    printf("\nYou have 2 minutes to connect, you must disconnect before the above time.\n");
    printf("\nList file of you:\n");
    int listen_sock, conn_sock;
    struct sockaddr_in server;
    struct sockaddr_in client;
    int sin_size;
    char buffer[BUFF_SIZE], nameFile[BUFF_SIZE];
    Message send_msg, recv_msg;
    int bytes_sent, bytes_received, count = 0;
    FILE *fp = NULL;
    char hashFileReturn[BUFF_SIZE];

    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = server_addr.sin_addr.s_addr;
    server.sin_port = htons(P2P_SERVER_PORT);

    // Step 4: P2P
    if (bind(listen_sock, (struct sockaddr *)&server, sizeof(struct sockaddr)) < 0)
    {
        perror("\nBind failed");
        // exit(EXIT_FAILURE);
    }
    if (listen(listen_sock, 5) < 0)
    {
        perror("listen");
        // exit(EXIT_FAILURE);
    }

    // accept request
    sin_size = sizeof(struct sockaddr_in);
    if ((conn_sock = accept(listen_sock, (struct sockaddr *)&client, &sin_size)) == -1)
        perror("\nError: ");

    memset(&send_msg, 0, sizeof(Message));

    send_msg.ms_type = CONNECT;
    send_msg.dt_type = STRING_LIST;

    for (int i = 0; i < countFileLst; i++)
    {

        strcpy(send_msg.value.filelst[i], fileLst[i]);
        printf("    %d.  %s\n", i + 1, send_msg.value.filelst[i]);

        send_msg.size = countFileLst;
    }
    bytes_sent = send(conn_sock, &send_msg, sizeof(send_msg), 0);

    while (1)
    {
        memset(&recv_msg, 0, sizeof(Message));

        bytes_received = recv(conn_sock, &recv_msg, sizeof(recv_msg), 0); // blocking
        if (bytes_received <= 0)
        {
            continue;
        }
        else
        {
            switch (recv_msg.ms_type)
            {
                // nhận được tín hiệu yêu cầu download file từ client kia
                // đọc file và gửi đi 
            case SELECT_FILE:
                count = 0;
                memset(buffer, '\0', (strlen(buffer)) + 1);
                memset(hashFileReturn, '\0', sizeof(char) * BUFF_SIZE);
                memset(&send_msg, 0, sizeof(Message));
                memset(nameFile, '\0', sizeof(char) * BUFF_SIZE);

                printf("The other client choose file %s to download.\n", recv_msg.value.buff);
                strcpy(nameFile, "doc/");
                strcat(nameFile, recv_msg.value.buff);

                fp = fopen(nameFile, "r");

                send_msg.ms_type = SEND_FILE;
                send_msg.dt_type = STRING;

                // cứ đọc 10 dòng sẽ gửi 1 lần.
                while (fgets(buffer, 1024, fp) != NULL)
                {
                    count++;
                    strcat(send_msg.value.buff, buffer);
                    if (count % 10 == 0)
                    {
                        bytes_sent = send(conn_sock, &send_msg, sizeof(send_msg), 0);
                        memset(&send_msg, 0, sizeof(Message));
                    }
                    if (bytes_sent <= 0)
                    {
                        printf("Connection closed!\n");
                        exit(0);
                    }
                    memset(buffer, '\0', (strlen(buffer)) + 1);
                }
                fclose(fp);

                bytes_sent = send(conn_sock, &send_msg, sizeof(send_msg), 0);

                printf("\nSent the content of file %s :\n%s \n", nameFile, send_msg.value.buff);
                memset(&send_msg, 0, sizeof(Message));

                // sau khi kết thúc thì gửi hash file kèm tín hiệu end file 
                // để client kia biết đã gửi xong file
                hashFile(nameFile, hashFileReturn);
                strcpy(send_msg.value.buff, hashFileReturn);
                send_msg.dt_type = STRING;
                send_msg.ms_type = END_FILE;
                bytes_sent = send(conn_sock, &send_msg, sizeof(send_msg), 0);

                break;
            case END_CONNECT:

                send_msg.ms_type = END_CONNECT;
                send_msg.dt_type = NONE;

                bytes_sent = send(client_sock, &send_msg, sizeof(send_msg), 0);

                break;
            default:
                break;
            }
        }

        if (recv_msg.ms_type == END_CONNECT)
            break;
    }
    close(listen_sock);
}

void *sendRequestToServer(void *vargp)
{
    char fileName[BUFF_SIZE], port[BUFF_SIZE], address[BUFF_SIZE];
    int bytes_sent;
    Message ms;
    pthread_t pid;
    int choice, choiceOfConnect, choiceClient = 0;
    printf("\nWelcome to P2P file tranfer application\n");
    while (1)
    {
        printf("\n---------------Menu Request Server---------------\n");
        printf("1. Input file name to search\n");
        printf("2. Input IP address and Port of client to request connect\n");
        printf("3. Answer connection request\n");
        printf("4. Exit\n");
        printf("-------------------------------------------------\n\n");

        printf("(Send server) Please enter your choice: ");
        scanf("%d", &choice);

        // while (getchar() != '\n');

        switch (choice)
        {
        case 1:
        {
            printf("\n(Send server)Input file name and press ENTER to search:\n");

            memset(fileName, '\0', (strlen(fileName)) + 1);
            fgets(fileName, BUFF_SIZE, stdin);
            fileName[strlen(fileName) - 1] = '\0';
            while (getchar() != '\n')
                ;

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
            while (getchar() != '\n')
                ;

            printf("\n(Send server)Input IP address Of client:\n");

            memset(address, '\0', (strlen(address)) + 1);
            fgets(address, BUFF_SIZE, stdin);
            address[strlen(address) - 1] = '\0';

            printf("\nInput Port Of client:\n");

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
            sleep(20);
            if (waitForReplyFromOtherClient)
            {
                sleep(120);
            }

            break;

            // cần phải trả lời đồng ý hay từ chối cho kết nối ? và kết nối với client nào, thì nhập vào từ bàn phím
            // phải trả lời cho từng trường hợp đồng ý hay từ chối

        case 3:
            // port và Ip address này phải thuộc mảng các client đã gửi yêu cầu kết nối , nếu không thì ,
            memset(&ms, 0, sizeof(Message));
            ms.ms_type = RES_SELECT;

            if (countClientReqConn <= 0)
            {
                printf("\nNo connection request from other client.\n");
            }
            else
            {
                printf("\n(Send server)Clients request connection:\n");
                for (int i = 0; i < countClientReqConn; i++)
                {
                    // printf(" %d - %d\n", atoi(port), ntohs(clientReqConn[i].sin_port));
                    // printf(" %s - %s\n", inet_ntoa(ms.value.addr.sin_addr), inet_ntoa(clientReqConn[i].sin_addr));
                    printf("    %d. %s:%d\n", i + 1, inet_ntoa(clientReqConn[i].sin_addr), ntohs(clientReqConn[i].sin_port));
                }
                printf("\nThe client you choose is: ");
                scanf("%d", &choiceClient);

                if (choiceClient > countClientReqConn)
                {
                    printf("Invalid selection.\n");
                    continue;
                }
                else
                {
                    ms.value.addr.sin_family = AF_INET;
                    ms.value.addr.sin_port = clientReqConn[choiceClient - 1].sin_port;
                    ms.value.addr.sin_addr.s_addr = clientReqConn[choiceClient - 1].sin_addr.s_addr;
                }

                printf("\n    1. Accept\n");
                printf("    2. Reject\n\n");
                printf("Your choice is : ");
                scanf("%d", &choiceOfConnect);

                if (choiceOfConnect == 1)
                {
                    ms.dt_type = ADDRESS;
                    bytes_sent = send(client_sock, &ms, sizeof(ms), 0);

                    pthread_create(&threadOfClientReceiveConn, NULL, clientRecvReqDown, NULL);
                    sleep(180);
                    pthread_cancel(threadOfClientReceiveConn);
                }
                else
                {
                    ms.dt_type = NONE;
                    bytes_sent = send(client_sock, &ms, sizeof(ms), 0);
                }
            }

            break;

        case 4:
            // Thoát nhưng chưa xóa dữ liệu bên server
            printf("Goodbye.\n");
            memset(&ms, 0, sizeof(Message));
            ms.ms_type = END;
            ms.dt_type = NONE;
            bytes_sent = send(client_sock, &ms, sizeof(ms), 0);
            exit(0);
            break;

        default:
            printf("Choice does not exist.\n");
        }
    }
}

int main()
{
    struct dirent *de;
    DIR *dr;
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

    pthread_create(&thread_id, NULL, sendRequestToServer, NULL);

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
                    printf("Could not open current directory.\n");
                    break;
                }
                memset(fileLst, 0, sizeof(fileLst));

                countFileLst = 0;

                while ((de = readdir(dr)) != NULL)
                {
                    if (de->d_type == 8)
                    {
                        strcpy(send_msg.value.filelst[i++], de->d_name);
                        strcpy(fileLst[countFileLst++], de->d_name);
                    }

                    send_msg.size = i;
                    if (i >= MAX_LEN)
                    {
                        printf("Out of array length\n");
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
                clientReqConn[countClientReqConn].sin_family = AF_INET;
                clientReqConn[countClientReqConn].sin_port = ms.value.addr.sin_port;
                clientReqConn[countClientReqConn].sin_addr.s_addr = ms.value.addr.sin_addr.s_addr;

                countClientReqConn++;
                break;

            case RES_SELECT:
                // printf("\nres_select: %s\n",ms.value.buff);
                printf("\nResponse of request connection: ");
                if (!strcmp(ms.value.buff, "yes"))
                {
                    waitForReplyFromOtherClient = true;
                    printf("Aceept.\n\n");

                    pthread_create(&threadOfClientRequestConn, NULL, clientSendReqDown, NULL);
                    sleep(180);
                    pthread_cancel(threadOfClientRequestConn);
                }
                else if (!strcmp(ms.value.buff, "no"))
                {
                    printf("Reject.\n\n");
                    waitForReplyFromOtherClient = false;
                }
                else if (!strcmp(ms.value.buff, "busy"))
                {
                    printf("That client is busy.\n\n");
                    waitForReplyFromOtherClient = false;
                }
                else
                {
                    printf("No matching client found.\n");
                    waitForReplyFromOtherClient = false;
                }

                break;
            default:
                break;
            }
        }
    }
    pthread_join(thread_id, NULL);

    close(client_sock);
    return 0;
}

// Công việc cần làm thêm : không hiển thị thông tin của mình
// - mở cổng để kết nối, có thể kết nối theo mô hình client server
// - các client thoát phải xóa dữ liệu trong list

// tải xong thì đóng connect, và
